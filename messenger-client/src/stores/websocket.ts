import { defineStore } from 'pinia'
import { ref, shallowRef } from 'vue'
import { useAuthStore } from './auth'
import { useChatsStore } from './chat'
import { messenger } from '@/proto/message'
import type { Message } from '@/types/message'
import { getOutbox, removeFromOutbox } from '@/utils/outbox'
import _sodium from 'libsodium-wrappers'

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

export const useWebSocketStore = defineStore('websocket', () => {
  const auth = useAuthStore()
  const chatsStore = useChatsStore()

  const status = ref<ConnectionStatus>('disconnected')
  const ws = shallowRef<WebSocket | null>(null)
  let sharedKey: Uint8Array | null = null
  let myKeypair: { publicKey: Uint8Array; privateKey: Uint8Array } | null = null
  let cryptoReady = false
  let cryptoPromise: Promise<void> | null = null
  let connectTimeout: ReturnType<typeof setTimeout> | null = null
  let keyExchangeTimeout: ReturnType<typeof setTimeout> | null = null
  let connectionGeneration = 0
  let keyExchangeAttempts = 0
  const MAX_ATTEMPTS = 3

  async function initCrypto() {
    if (cryptoReady) return
    if (cryptoPromise) return cryptoPromise
    cryptoPromise = (async () => {
      await _sodium.ready
      myKeypair = _sodium.crypto_box_keypair()
      cryptoReady = true
    })()
    return cryptoPromise
  }

  function clearConnectTimeout() {
    if (connectTimeout) {
      clearTimeout(connectTimeout)
      connectTimeout = null
    }
  }

  function clearKeyExchangeTimeout() {
    if (keyExchangeTimeout) {
      clearTimeout(keyExchangeTimeout)
      keyExchangeTimeout = null
    }
  }

  function disconnect() {
    clearConnectTimeout()
    clearKeyExchangeTimeout()
    connectionGeneration++
    if (ws.value) {
      ws.value.close()
      ws.value = null
    }
    sharedKey = null
    status.value = 'disconnected'
  }

  function sendPublicKey() {
    if (!myKeypair || !ws.value || ws.value.readyState !== WebSocket.OPEN) return
    console.log('Sending public key, attempt', keyExchangeAttempts + 1)
    ws.value.send(myKeypair.publicKey as BufferSource)
    keyExchangeAttempts++
    clearKeyExchangeTimeout()
    keyExchangeTimeout = setTimeout(() => {
      if (!sharedKey && ws.value?.readyState === WebSocket.OPEN && keyExchangeAttempts < MAX_ATTEMPTS) {
        sendPublicKey()
      } else if (!sharedKey) {
        console.error('Key exchange failed after', MAX_ATTEMPTS, 'attempts')
        status.value = 'error'
      }
    }, 5000)
  }

  // ==========================================================
  //  Автопереотправка «зависших» сообщений после реконнекта
  // ==========================================================
  function flushOutbox() {
    const outbox = getOutbox()
    for (const [chatId, list] of Object.entries(outbox)) {
      for (const m of list) {
        if (m.status !== 'error' && m.status !== 'pending') continue
        const recipient = getRecipientIdFromChat(chatId)
        if (!recipient) continue
        // Обновим статус на pending и попробуем
        chatsStore.updateMessageStatus(chatId, m.id!, 'pending')
        const ok = sendMessage(recipient, m.text)
        const next: Message['status'] = ok ? 'sent' : 'error'
        chatsStore.updateMessageStatus(chatId, m.id!, next)
        if (ok) {
          // статус 'sent' - помечаем в outbox, чтобы не переотправлять
          // (но не удаляем — удалим, когда придёт серверное эхо)
          updateOutboxStatusLocal(chatId, m.id!, 'sent')
        }
      }
    }
  }

  function updateOutboxStatusLocal(chatId: string, msgId: string, status: Message['status']) {
    // лёгкий wrapper: не тянем лишний импорт
    try {
      const KEY = 'messenger_outbox'
      const ob = JSON.parse(localStorage.getItem(KEY) || '{}')
      const idx = (ob[chatId] || []).findIndex((m: Message) => m.id === msgId)
      if (idx !== -1) {
        ob[chatId][idx].status = status
        localStorage.setItem(KEY, JSON.stringify(ob))
      }
    } catch { /* ignore */ }
  }

  function getRecipientIdFromChat(chatId: string): string | null {
    const parts = chatId.split('_')
    if (parts.length !== 3) return null
    const [, a, b] = parts
    if (!auth.userId) return null
    return a === auth.userId ? b : a
  }

  async function connect(token?: string) {
    await initCrypto()
    if (!myKeypair) {
      console.error('Crypto not initialized')
      status.value = 'error'
      return
    }

    if (ws.value) {
      connectionGeneration++
      clearKeyExchangeTimeout()
      ws.value.close()
      ws.value = null
    }

    clearConnectTimeout()
    sharedKey = null
    status.value = 'connecting'

    const useToken = token || auth.token
    if (!useToken) {
      status.value = 'error'
      return
    }

    const socket = new WebSocket(`wss://127.0.0.1:9000/ws?token=${encodeURIComponent(useToken)}`)
    const generation = ++connectionGeneration
    socket.binaryType = 'arraybuffer'
    ws.value = socket

    socket.onopen = () => {
      if (generation !== connectionGeneration) return
      console.log('WebSocket open, readyState:', socket.readyState)
      clearConnectTimeout()
      if (myKeypair) {
        keyExchangeAttempts = 0
        sendPublicKey()
      } else {
        status.value = 'error'
      }
    }

    socket.onmessage = async (event) => {
      if (generation !== connectionGeneration) return
      if (!(event.data instanceof ArrayBuffer)) return
      const data = new Uint8Array(event.data)

      // ---- 1) Key exchange ----
      if (!sharedKey) {
        if (data.length !== _sodium.crypto_box_PUBLICKEYBYTES) return
        try {
          sharedKey = _sodium.crypto_box_beforenm(data, myKeypair!.privateKey)
          status.value = 'connected'
          clearConnectTimeout()
          clearKeyExchangeTimeout()
          // Реконнект завершён → пробуем отправить застрявшие
          flushOutbox()
        } catch (e) {
          console.error('Shared key computation failed', e)
          status.value = 'error'
        }
        return
      }

      // ---- 2) Decrypt ----
      try {
        const envelope = messenger.SecureEnvelope.decode(data)
        const ciphertext = envelope.ciphertext as Uint8Array
        const nonce = envelope.nonce as Uint8Array
        const sender = envelope.senderId || 'unknown'
        const recipient = envelope.recipientId || ''

        const decrypted = _sodium.crypto_box_open_easy_afternm(ciphertext, nonce, sharedKey)
        const text = new TextDecoder().decode(decrypted)

        const chatId = getChatId(sender, recipient)
        const isMine = sender === auth.userId

        // ==========================================================
        //  3a) ЭХО нашего же сообщения - реконсиляция
        // ==========================================================
        if (isMine) {
          const list = chatsStore.messagesMap[chatId] || []
          const now = Date.now()
          const idx = list.findIndex(m =>
            (m.status === 'pending' || m.status === 'sent' || m.status === 'error') &&
            m.text === text &&
            Math.abs(m.timestamp - now) < 60_000,
          )
          if (idx !== -1) {
            const prev = list[idx]
            chatsStore.updateMessageStatus(chatId, prev.id!, 'delivered')
            removeFromOutbox(chatId, prev.id!)
          }
          return
        }

        // ==========================================================
        //  3b) Входящее
        // ==========================================================
        const msg: Message = {
          sender,
          text,
          timestamp: Date.now(),
          recipient,
          status: 'delivered',
        }

        chatsStore.addMessage(chatId, msg)

        if (!chatsStore.chatList.find(c => c.id === chatId)) {
          const otherId = sender
          chatsStore.addChat(chatId, `User ${otherId}`)
          // асинхронно синхронизируемся со списком с сервера
          chatsStore.loadChats().catch(() => {})
        }

        // В фоновом режиме (чат не активен) — инкремент unread
        if (chatsStore.activeChatId !== chatId) {
          const chat = chatsStore.chatList.find(c => c.id === chatId)
          if (chat) chat.unread = (chat.unread || 0) + 1
        }
      } catch (err) {
        console.error('Decryption error', err)
      }
    }

    socket.onerror = (err) => {
      if (generation !== connectionGeneration) return
      console.error('WebSocket error:', err)
    }

    socket.onclose = (event) => {
      if (generation !== connectionGeneration) return
      console.log(`WebSocket closed: code ${event.code}, reason: ${event.reason}`)
      clearConnectTimeout()
      clearKeyExchangeTimeout()
      status.value = event.code === 1000 ? 'disconnected' : 'error'
      ws.value = null
      sharedKey = null
    }
  }

  function sendMessage(recipientId: string, text: string): boolean {
    if (!sharedKey || !ws.value || ws.value.readyState !== WebSocket.OPEN) {
      console.warn('Cannot send: missing sharedKey or ws not open')
      return false
    }
    const nonce = _sodium.randombytes_buf(_sodium.crypto_box_NONCEBYTES)
    const ciphertext = _sodium.crypto_box_easy_afternm(
      new TextEncoder().encode(text),
      nonce,
      sharedKey,
    )
    const envelope = messenger.SecureEnvelope.create({
      ciphertext,
      nonce,
      senderId: auth.userId || '',
      recipientId,
    })
    const binary = messenger.SecureEnvelope.encode(envelope).finish()
    ws.value.send(binary as BufferSource)
    return true
  }

  function getChatId(sender: string, recipient: string) {
    const ids = [parseInt(sender), parseInt(recipient)].sort((a, b) => a - b)
    return `chat_${ids[0]}_${ids[1]}`
  }

  initCrypto()

  return {
    status,
    connect,
    disconnect,
    sendMessage,
  }
})