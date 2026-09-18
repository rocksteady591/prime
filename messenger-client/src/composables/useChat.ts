import { computed, readonly, Ref, ref, watch } from 'vue';
import { useChatsStore } from '@/stores/chat';
import { useAuthStore } from '@/stores/auth';
import { useWebSocketStore } from '@/stores/websocket';
import { getRecipientId, getChatId } from '@/utils/chat';
import type { Message } from '@/types/message';

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

  // Отправка сообщения
  function sendMessage(text: string): boolean {
    const trimmed = text.trim();
    if (!trimmed) return false;
    const chatId = chatIdRef.value;
    if (!chatId) {
      console.warn('Нет активного чата');
      return false;
    }
    if (!authStore.userId) {
      console.warn('Пользователь не авторизован');
      return false;
    }
    const recipient = recipientId.value;
    if (!recipient) {
      console.warn('Не удалось определить получателя');
      return false;
    }

    const tempMsg: Message = {
      id: `temp_${Date.now()}_${Math.random()}`,
      sender: authStore.userId,
      text: trimmed,
      timestamp: Date.now(),
      status: 'pending',
    };

    chatsStore.addMessage(chatId, tempMsg)

    const sent = wsStore.sendMessage(recipient, trimmed);
    if (!sent) {
      chatsStore.updateMessageStatus(chatId, tempMsg.id!, 'error');
    }
    return sent;
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
      // Определяем offset как текущее количество сообщений
      const offset = currentMessages.length;
      await chatsStore.loadHistory(chatId, limit, offset, true); // append = true
    } finally {
      loadingMore.value = false;
    }
  }

  // Метод для повторной отправки сообщения с ошибкой
  function resendMessage(msgId: string) {
    const chatId = chatIdRef.value;
    if (!chatId) return;
    const msgs = chatsStore.messagesMap[chatId];
    if (!msgs) return;
    const msg = msgs.find(m => m.id === msgId);
    if (!msg || msg.status !== 'error') return;
    if (!authStore.userId) return;

    // Меняем статус на pending
    chatsStore.updateMessageStatus(chatId, msgId, 'pending');

    // Повторно отправляем
    const recipient = recipientId.value;
    if (!recipient) {
      chatsStore.updateMessageStatus(chatId, msgId, 'error');
      return;
    }
    const sent = wsStore.sendMessage(recipient, msg.text);
    if (!sent) {
      chatsStore.updateMessageStatus(chatId, msgId, 'error');
    } else {
      // Если нужно обновить статус на sent позже – можно подписаться на подтверждение
      // Пока оставляем как есть, статус изменится при получении подтверждения от сервера
    }
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