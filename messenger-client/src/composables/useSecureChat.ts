import { ref, shallowRef, readonly, type Ref } from 'vue';
import _sodium from 'libsodium-wrappers';
import { messenger } from '../proto/message.js';

interface Message {
  sender: string;
  text: string;
  timestamp: number;
}

type ConnectionStatus = 'disconnected' | 'connecting' | 'connected' | 'error';

export function useSecureChat() {
  const messages: Ref<Message[]> = ref([]);
  const status = ref<ConnectionStatus>('disconnected');
  const ws = shallowRef<WebSocket | null>(null);

  let sharedKey: Uint8Array | null = null;
  let myKeypair: {
    publicKey: Uint8Array;
    privateKey: Uint8Array;
  } | null = null;

  const senderId = `user-${Math.random().toString(36).substr(2, 8)}`;

  async function initCrypto(): Promise<void> {
    await _sodium.ready;
    myKeypair = _sodium.crypto_box_keypair();
  }

  function connect(url: string = 'ws://localhost:9000'): void {
    if (ws.value) ws.value.close();

    status.value = 'connecting';
    const socket = new WebSocket(url);
    socket.binaryType = 'arraybuffer';
    ws.value = socket;

    socket.onopen = async () => {
      if (!myKeypair) {
        console.error('Keypair not initialized');
        return;
      }
      // Отправляем публичный ключ, приводим к совместимому типу
      socket.send(myKeypair.publicKey as BufferSource);
    };

    socket.onmessage = async (event: MessageEvent) => {
      if (!(event.data instanceof ArrayBuffer)) {
        console.error('Expected binary data');
        return;
      }

      const data = new Uint8Array(event.data);

      if (!sharedKey) {
        if (!myKeypair) {
          console.error('Keypair not initialized');
          return;
        }
        const serverPk = data;
        sharedKey = _sodium.crypto_box_beforenm(serverPk, myKeypair.privateKey);
        status.value = 'connected';
        console.log('Shared key established');
        return;
      }

      try {
        const envelope = messenger.SecureEnvelope.decode(data);
        // Приводим поля: в реальности они Uint8Array, но типы могут говорить string
        const ciphertext = envelope.ciphertext || new Uint8Array(0);
        const nonce = envelope.nonce || new Uint8Array(0);
        const sender = envelope.senderId || 'unknown';

        const decrypted = _sodium.crypto_box_open_easy_afternm(
          ciphertext,
          nonce,
          sharedKey
        );
        messages.value.push({
          sender,
          text: new TextDecoder().decode(decrypted),
          timestamp: Date.now(),
        });
      } catch (err) {
        console.error('Decryption error', err);
      }
    };

    socket.onerror = () => {
      status.value = 'error';
    };
    socket.onclose = () => {
      status.value = 'disconnected';
      sharedKey = null;
      setTimeout(() => connect(url), 3000);
    };
  }

  async function sendMessage(text: string): Promise<void> {
    if (!sharedKey || !ws.value || ws.value.readyState !== WebSocket.OPEN) {
      console.warn('Not ready to send');
      return;
    }

    const nonce = _sodium.randombytes_buf(_sodium.crypto_box_NONCEBYTES);
    const ciphertext = _sodium.crypto_box_easy_afternm(
      new TextEncoder().encode(text),
      nonce,
      sharedKey
    );

    const envelope = messenger.SecureEnvelope.create({
        ciphertext,
        nonce,
        senderId,
    });

    const binary = messenger.SecureEnvelope.encode(envelope).finish();
    ws.value.send(binary);
  }

  initCrypto();

  return {
    messages: readonly(messages),
    status: readonly(status),
    connect,
    sendMessage,
  };
}