import { defineStore } from 'pinia'
import { ref, shallowRef, readonly } from 'vue'
import { useAuthStore } from './auth'
import { useChatsStore } from './chat'
import { messenger } from '@/proto/message'
import type { Message } from '@/types/message'
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
    clearConnectTimeout();
    clearKeyExchangeTimeout();
    connectionGeneration++
    if (ws.value) {
      ws.value.close()
      ws.value = null
    }
    sharedKey = null
    status.value = 'disconnected'
  }

  let keyExchangeAttempts = 0;
const MAX_ATTEMPTS = 3;

  function sendPublicKey() {
      if (!myKeypair || !ws.value || ws.value.readyState !== WebSocket.OPEN) return;
      console.log('Sending public key, attempt', keyExchangeAttempts + 1);
      ws.value?.send(myKeypair.publicKey as BufferSource);
      keyExchangeAttempts++;
      // Если через 5 секунд нет sharedKey, повторить
        clearKeyExchangeTimeout();
      keyExchangeTimeout = setTimeout(() => {
          if (!sharedKey && ws.value?.readyState === WebSocket.OPEN && keyExchangeAttempts < MAX_ATTEMPTS) {
              sendPublicKey();
          } else if (!sharedKey) {
              console.error('Key exchange failed after', MAX_ATTEMPTS, 'attempts');
              status.value = 'error';
          }
      }, 5000);
  }

  async function connect(token?: string) {
    await initCrypto()
    if (!myKeypair) {
      console.error('Crypto not initialized');
      status.value = 'error'
      return
    }
    console.log('myKeypair is ready:', myKeypair);

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

    const socket = new WebSocket(`wss://127.0.0.1:9000/ws?token=${encodeURIComponent(useToken)}`);
    const generation = ++connectionGeneration
    socket.binaryType = 'arraybuffer'
    ws.value = socket

    socket.onopen = () => {
      if (generation !== connectionGeneration) return
        console.log('WebSocket open, readyState:', socket.readyState);
        clearConnectTimeout();
        if (myKeypair) {
            keyExchangeAttempts = 0;
            sendPublicKey();
        } else {
            status.value = 'error';
        }
    };

    socket.onmessage = async (event) => {
      if (generation !== connectionGeneration) return
      console.log('onmessage, data type:', typeof event.data, 'byteLength:', event.data?.byteLength);
      if (!(event.data instanceof ArrayBuffer)) {
          console.warn('Received non-binary data, ignoring');
          return;
      }
      const data = new Uint8Array(event.data)

      if (!sharedKey) {
        const expectedLen = _sodium.crypto_box_PUBLICKEYBYTES
        if (data.length !== expectedLen) return
        try {
          sharedKey = _sodium.crypto_box_beforenm(data, myKeypair!.privateKey)
          status.value = 'connected'
          clearConnectTimeout()
        } catch (e) {
          console.error('Shared key computation failed', e)
          status.value = 'error'
        }
        return
      }

      try {
        console.log('Received message, length:', data.length)
        const envelope = messenger.SecureEnvelope.decode(data)
        const ciphertext = envelope.ciphertext as Uint8Array
        const nonce = envelope.nonce as Uint8Array
        const sender = envelope.senderId || 'unknown'
        const recipient = envelope.recipientId || ''

        const decrypted = _sodium.crypto_box_open_easy_afternm(ciphertext, nonce, sharedKey)
        const text = new TextDecoder().decode(decrypted)
        console.log('Decrypted message:', text, 'sender:', sender, 'recipient:', recipient)
        const msg: Message = {
          sender,
          text,
          timestamp: Date.now(),
          recipient,
        }

        const chatId = getChatId(sender, recipient)
        chatsStore.addMessage(chatId, msg)
        if (!chatsStore.chatList.find(c => c.id === chatId)) {
          const otherId = sender === auth.userId ? recipient : sender
          chatsStore.addChat(chatId, `User ${otherId}`)
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
      clearConnectTimeout();
      clearKeyExchangeTimeout();
      if (event.code === 1000) {
        status.value = 'disconnected'
      } else {
        status.value = 'error'
      }
      ws.value = null
      sharedKey = null
    }
  }

  function sendMessage(recipientId: string, text: string): boolean {
    console.log('sendMessage called', { recipientId, text, sharedKey: !!sharedKey, wsReady: ws.value?.readyState === WebSocket.OPEN })
    if (!sharedKey || !ws.value || ws.value.readyState !== WebSocket.OPEN) {
        console.warn('Cannot send: missing sharedKey or ws not open')
        return false
    }
    const nonce = _sodium.randombytes_buf(_sodium.crypto_box_NONCEBYTES)
    const ciphertext = _sodium.crypto_box_easy_afternm(
      new TextEncoder().encode(text),
      nonce,
      sharedKey
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

  // Инициализация криптографии
  initCrypto()

  return {
    status,  
    connect,
    disconnect,
    sendMessage,
  }
})