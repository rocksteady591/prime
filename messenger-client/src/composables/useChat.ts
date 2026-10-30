import { computed, readonly, Ref, ref, watch } from 'vue';
import { useChatsStore } from '@/stores/chat';
import { useAuthStore } from '@/stores/auth';
import { useWebSocketStore } from '@/stores/websocket';
import { getRecipientId, getChatId } from '@/utils/chat';
import { addToOutbox, updateOutboxStatus } from '@/utils/outbox';
import type { Attachment, Message } from '@/types/message';

const ECHO_TIMEOUT_MS = 10_000

export function useChat(chatIdRef: Ref<string | null>) {
  const chatsStore = useChatsStore();
  const authStore = useAuthStore();
  const wsStore = useWebSocketStore();

  watch(
    chatIdRef,
    (newId, oldId) => {
      if (newId && newId !== oldId) {
        if (!chatsStore.messagesMap[newId] || chatsStore.messagesMap[newId].length === 0) {
          chatsStore.loadHistory(newId);
        }
      }
    },
    { immediate: true }
  );

  const recipientId = computed(() => {
    const chatId = chatIdRef.value;
    if (!chatId || !authStore.userId) return null;
    return getRecipientId(chatId, authStore.userId);
  });

  const messages = computed(() => {
    const chatId = chatIdRef.value;
    return chatId ? chatsStore.messagesMap[chatId] || [] : [];
  });
  const chatName = computed(() => {
    const chatId = chatIdRef.value;
    if (!chatId) return 'Чат';
    const chat = chatsStore.chatList.find(c => c.id === chatId);
    return chat?.name || 'Чат';
  });

  // ==========================================================
  //  Отправка сообщения
  //  pending → (ws.send успешен) → sent  |  иначе → error
  //  Всё сразу дублируется в outbox (localStorage) для
  //  восстановления после F5.
  // ==========================================================
  function sendMessage(text: string, attachments: Attachment[] = []): boolean {
    const trimmed = text.trim()
    if (!trimmed && !attachments.length) return false
    const chatId = chatIdRef.value
    if (!chatId || !authStore.userId) return false
    const recipient = recipientId.value
    if (!recipient) return false

    const tempMsg: Message = {
      id: `temp_${Date.now()}_${Math.random().toString(36).slice(2)}`,
      sender: authStore.userId,
      text: trimmed,
      timestamp: Date.now(),
      status: 'pending',                    // ← НЕ 'sent'
      attachments: attachments.length ? attachments : undefined,
    }

    chatsStore.addMessage(chatId, tempMsg)
    addToOutbox(chatId, tempMsg)

    const queued = wsStore.sendMessage(recipient, trimmed)

    if (!queued) {
      // ws не открыт / sharedKey отсутствует — сразу error
      chatsStore.updateMessageStatus(chatId, tempMsg.id!, 'error')
      updateOutboxStatus(chatId, tempMsg.id!, 'error')
      return false
    }

    // Ждём эха. Если за 10с не пришло — сервер, скорее всего, не получил.
    scheduleEchoTimeout(chatId, tempMsg.id!)
    return true
  }

  function scheduleEchoTimeout(chatId: string, msgId: string) {
    setTimeout(() => {
      const msgs = chatsStore.messagesMap[chatId]
      if (!msgs) return
      const m = msgs.find(x => x.id === msgId)
      if (m && m.status === 'pending') {
        console.warn('[useChat] echo timeout → error', msgId)
        chatsStore.updateMessageStatus(chatId, msgId, 'error')
        updateOutboxStatus(chatId, msgId, 'error')
      }
    }, ECHO_TIMEOUT_MS)
  }

  const loadingMore = ref(false);

  // Загрузка истории с пагинацией
  async function loadMore(limit = 50) {
    const chatId = chatIdRef.value;
    if (!chatId || loadingMore.value) return;
    const currentMessages = messages.value;
    if (currentMessages.length === 0) return;

    loadingMore.value = true;
    try {
      const offset = currentMessages.length;
      await chatsStore.loadHistory(chatId, limit, offset, true);
    } finally {
      loadingMore.value = false;
    }
  }

  // ==========================================================
  //  Повторная отправка сообщения со статусом 'error'
  // ==========================================================
  function resendMessage(msgId: string) {
    const chatId = chatIdRef.value;
    if (!chatId) return;
    if (!authStore.userId) return;

    const msgs = chatsStore.messagesMap[chatId];
    if (!msgs) return;

    const msg = msgs.find(m => m.id === msgId);
    if (!msg || msg.status !== 'error') return;

    const recipient = recipientId.value;
    if (!recipient) return;

    // pending на время переотправки — на случай, если кто-то
    // подписан на статус (спиннер, анимация, ...)
    chatsStore.updateMessageStatus(chatId, msgId, 'pending');
    updateOutboxStatus(chatId, msgId, 'pending');

    const sent = wsStore.sendMessage(recipient, msg.text);

    const nextStatus: Message['status'] = sent ? 'sent' : 'error';
    chatsStore.updateMessageStatus(chatId, msgId, nextStatus);
    updateOutboxStatus(chatId, msgId, nextStatus);
  }

  return {
    recipientId: readonly(recipientId),
    messages: readonly(messages),
    chatName: readonly(chatName),
    sendMessage,
    loadMore,
    resendMessage,
    loadingMore: readonly(loadingMore),
  };
}