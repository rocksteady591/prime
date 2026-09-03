import { computed, ref, watch } from 'vue';
import { useChatsStore } from '@/stores/chat';
import { useAuthStore } from '@/stores/auth';
import { useWebSocketStore } from '@/stores/websocket';
import { getRecipientId, getChatId } from '@/utils/chat';
import type { Message } from '@/types/message';

export function useChat(chatId: string) {
  const chatsStore = useChatsStore();
  const authStore = useAuthStore();
  const wsStore = useWebSocketStore();

  const recipientId = computed(() => {
    if (!authStore.userId) return null;
    return getRecipientId(chatId, authStore.userId);
  });

  const messages = computed(() => chatsStore.messagesMap[chatId] || []);
  const chatName = computed(() => {
    const chat = chatsStore.chatList.find(c => c.id === chatId);
    return chat?.name || 'Чат';
  });

  // Отправка сообщения
  function sendMessage(text: string): boolean {
    const trimmed = text.trim();
    if (!trimmed) return false;
    if (!authStore.userId) return false;
    if (!recipientId.value) {
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

    chatsStore.addMessage(chatId, tempMsg);

    const sent = wsStore.sendMessage(recipientId.value, trimmed);
    if (sent) {
      // можно обновить статус позже, когда придёт подтверждение
      // пока оставим как есть
    } else {
      // пометить сообщение как ошибка
      const lastMsg = chatsStore.messagesMap[chatId]?.slice(-1)[0];
      if (lastMsg && lastMsg.id === tempMsg.id) {
        lastMsg.status = 'error';
      }
    }
    return sent;
  }

  // Загрузка истории с пагинацией (можно добавить позже)
  function loadMore(limit = 50, offset = 0) {
    chatsStore.loadHistory(chatId, limit, offset);
  }

  return {
    recipientId,
    messages,
    chatName,
    sendMessage,
    loadMore,
  };
}