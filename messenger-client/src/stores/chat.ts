import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { useAuthStore } from './auth'
import { apiFetch } from '@/utils/api'
import { Message } from '@/types/message'

export const useChatsStore = defineStore('chats', () => {
  const auth = useAuthStore()
  const chatList = ref<Array<{ id: string; name: string; unread?: number }>>([])
  const activeChatId = ref<string | null>(null)
  const messagesMap = ref<Record<string, Message[]>>({})
  const loadingHistory = ref<Record<string, boolean>>({});

   // Загрузка истории с возможностью добавления к уже имеющимся
  async function loadHistory(chatId: string, limit = 50, offset = 0, append = false) {
    const chat = chatList.value.find(c => c.id === chatId);
    if (!chat) return;
    
    const realChatId = (chat as any).raw?.id;
    if (realChatId == null) {
        console.warn('No raw chat id for', chatId);
        return;
    }

    loadingHistory.value[chatId] = true;
    try {
        const res = await apiFetch('/api/get_messages', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                Authorization: `Bearer ${auth.token}`,
            },
            body: JSON.stringify({
                chat_id: Number(realChatId),
                count_messages: limit,
                offset,
            }),
        });

        if (!res.ok) {
            throw new Error(res.data?.error || res.error || 'Failed to load messages');
        }

        const data = res.data || [];

        const msgs: Message[] = data.map((m: any) => ({
          id: m.msg_id?.toString(),
          sender: m.sender_id.toString(),
          text: m.text,
          timestamp: new Date(m.send_time).getTime(),
        }));

        if (!messagesMap.value[chatId]) {
          messagesMap.value[chatId] = [];
        }

        if (append) {
          messagesMap.value[chatId] = [...msgs, ...messagesMap.value[chatId]];
        } else {
          messagesMap.value[chatId] = msgs;
        }
    } catch (e) {
      console.error('Failed to load history', e);
      if (!append) {
        messagesMap.value[chatId] = [];
      }
    } finally {
      loadingHistory.value[chatId] = false;
    }
  }

  function updateMessageStatus(chatId: string, msgId: string, status: Message['status']) {
    const msgs = messagesMap.value[chatId];
    if (!msgs) return;
    const idx = msgs.findIndex(m => m.id === msgId);
    if (idx !== -1) {
      msgs[idx].status = status;
      // триггерим реактивность
      messagesMap.value[chatId] = [...msgs];
    }
  }

  const activeMessages = computed(() => {
    if (!activeChatId.value) return []
    return messagesMap.value[activeChatId.value] || []
  })

  const activeChatName = computed(() => {
    const chat = chatList.value.find(c => c.id === activeChatId.value)
    return chat?.name || 'Чат'
  })

  // Загрузка списка чатов пользователя (user_id берётся из хранилища)
  async function loadChats() {
    const userId = parseInt(auth.userId || '0')
    if (!userId) return

    try {
      const res = await apiFetch('/api/get_chats', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${auth.token}`,
        },
        body: JSON.stringify({}),
      })

      if (!res.ok) {
        // Безопасный доступ к полю сообщения с фолбэком
        throw new Error(res.data?.message || res.error || 'Failed to load chats')
      }

      const data = res.data || []

      const chats = data.map((c: any) => {
        const otherId = c.user1_id === userId ? c.user2_id : c.user1_id
        return {
          id: `chat_${Math.min(c.user1_id, c.user2_id)}_${Math.max(c.user1_id, c.user2_id)}`,
          name: `User ${otherId}`,
          raw: c,
        }
      })
      chatList.value = chats
    } catch (e) {
      console.error('Failed to load chats', e)
    }
  }

  function addMessage(chatId: string, msg: Message) {
    if (!messagesMap.value[chatId]) {
      messagesMap.value[chatId] = []
    }
    messagesMap.value[chatId].push(msg)
  }

  function addChat(chatId: string, name: string) {
    if (!chatList.value.find(c => c.id === chatId)) {
      chatList.value.push({ id: chatId, name })
    }
  }

  function setActiveChat(id: string) {
    activeChatId.value = id
    if (!messagesMap.value[id]) {
      loadHistory(id)
    }
  }

  return {
    chatList,
    activeChatId,
    messagesMap,
    activeMessages,
    activeChatName,
    loadChats,
    addMessage,
    addChat,
    setActiveChat,
    loadingHistory,
    loadHistory,
    updateMessageStatus,
  }
})