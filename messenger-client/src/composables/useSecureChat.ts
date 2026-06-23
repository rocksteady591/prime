// src/composables/useSecureChat.ts
import { ref, shallowRef, readonly, type Ref } from 'vue'
import _sodium from 'libsodium-wrappers'
import { messenger } from '../proto/message.js'

export interface Message {
  sender: string
  text: string
  timestamp: number
  recipient?: string
}

type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error'

// Принимаем userId извне (из authStore)
export function useSecureChat(userId: string) {
  const messages: Ref<Message[]> = ref([])
  const status = ref<ConnectionStatus>('disconnected')
  const ws = shallowRef<WebSocket | null>(null)

  let sharedKey: Uint8Array | null = null
  let myKeypair: {
    publicKey: Uint8Array
    privateKey: Uint8Array
  } | null = null

  async function initCrypto(): Promise<void> {
    await _sodium.ready
    myKeypair = _sodium.crypto_box_keypair()
  }

  function disconnect() {
      if (ws.value) {
        ws.value.close()
        ws.value = null
      }
      sharedKey = null
      status.value = 'disconnected'
    }

  function connect(token?: string): void {
    if (ws.value) ws.value.close()

    status.value = 'connecting'
    const wsUrl = token
      ? `ws://localhost:9000?token=${token}`
      : 'ws://localhost:9000'

    const socket = new WebSocket(wsUrl)
    socket.binaryType = 'arraybuffer'
    ws.value = socket

    socket.onopen = async () => {
      if (!myKeypair) {
        console.error('Keypair not initialized')
        return
      }
      socket.send(myKeypair.publicKey as BufferSource)
    }

    socket.onmessage = async (event: MessageEvent) => {
      if (!(event.data instanceof ArrayBuffer)) {
        console.error('Expected binary data')
        return
      }

      const data = new Uint8Array(event.data)

      if (!sharedKey) {
        if (!myKeypair) return
        const serverPk = data
        sharedKey = _sodium.crypto_box_beforenm(serverPk, myKeypair.privateKey)
        status.value = 'connected'
        console.log('Shared key established')
        return
      }

      try {
        const envelope = messenger.SecureEnvelope.decode(data)
        const ciphertext = (envelope.ciphertext as unknown as Uint8Array) || new Uint8Array(0)
        const nonce = (envelope.nonce as unknown as Uint8Array) || new Uint8Array(0)
        const sender = envelope.senderId || 'unknown'
        const recipient = envelope.recipientId || ''

        const decrypted = _sodium.crypto_box_open_easy_afternm(
          ciphertext,
          nonce,
          sharedKey!
        )

        messages.value.push({
          sender,
          text: new TextDecoder().decode(decrypted),
          timestamp: Date.now(),
          recipient
        })
      } catch (err) {
        console.error('Decryption error', err)
      }
    }

    socket.onerror = () => { status.value = 'error' }
    socket.onclose = () => {
      status.value = 'disconnected'
      sharedKey = null
      setTimeout(() => connect(token), 3000)
    }
  }

  async function sendMessage(text: string, recipientId: string): Promise<void> {
    if (!sharedKey || !ws.value || ws.value.readyState !== WebSocket.OPEN) {
      console.warn('Not ready to send')
      return
    }

    const nonce = _sodium.randombytes_buf(_sodium.crypto_box_NONCEBYTES)
    const ciphertext = _sodium.crypto_box_easy_afternm(
      new TextEncoder().encode(text),
      nonce,
      sharedKey
    )

    // Используем переданный userId как senderId
    const envelope = (messenger.SecureEnvelope as any).create({
      ciphertext,
      nonce,
      senderId: userId,
      recipientId
    })

    const binary = messenger.SecureEnvelope.encode(envelope).finish()
    ws.value.send(binary as BufferSource)
  }

  initCrypto()

  return {
    messages: readonly(messages),
    status: readonly(status),
    connect,
    disconnect,
    sendMessage,
    myUserId: userId 
  }
}