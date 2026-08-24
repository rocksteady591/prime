// stores/chat.ts
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface Message {
  sender: string
  text: string
  timestamp: number
  // можно добавить id, но в WebSocket мы не получаем id сообщения
}

export const useChatsStore = defineStore('chats', () => {
  const chatList = ref<Array<{ id: string; name: string; unread?: number }>>([])
  const activeChatId = ref<string | null>(null)
  const messagesMap = ref<Record<string, Message[]>>({})

  const activeMessages = computed(() => {
    if (!activeChatId.value) return []
    return messagesMap.value[activeChatId.value] || []
  })

  const activeChatName = computed(() => {
    const chat = chatList.value.find(c => c.id === activeChatId.value)
    return chat?.name || 'Чат'
  })

  // Загрузка списка чатов пользователя
  async function loadChats(userId: number) {
    try {
      const res = await fetch('/api/get_chats', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({ user_id: userId }),
      })
      const data = await res.json()
      if (!res.ok) throw new Error(data.message || 'Failed to load chats')
      // data: массив { id, user1_id, user2_id, create_timestamp }
      const chats = data.map((c: any) => {
        const otherId = c.user1_id === userId ? c.user2_id : c.user1_id
        return {
          id: `chat_${Math.min(c.user1_id, c.user2_id)}_${Math.max(c.user1_id, c.user2_id)}`,
          name: `User ${otherId}`, // позже можно заменить на реальное имя через /api/find_user
          raw: c,
        }
      })
      chatList.value = chats
    } catch (e) {
      console.error('Failed to load chats', e)
    }
  }

  // Загрузка истории сообщений для конкретного чата
  async function loadHistory(chatId: string, limit = 50, offset = 0) {
    // Находим raw чат с реальным id из БД
    const chat = chatList.value.find(c => c.id === chatId)
    if (!chat) return
    const realChatId = (chat as any).raw?.id
    if (!realChatId) return

    try {
      const res = await fetch('/api/get_messages', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${localStorage.getItem('auth_token')}`,
        },
        body: JSON.stringify({
          chat_id: realChatId,
          count_messages: limit,
          offset,
        }),
      })
      const data = await res.json()
      if (!res.ok) throw new Error(data.message || 'Failed to load messages')
      // data: массив { msg_id, chat_id, sender_id, text, send_time }
      const msgs = data.map((m: any) => ({
        sender: m.sender_id.toString(),
        text: m.text,
        timestamp: new Date(m.send_time).getTime(),
      }))
      messagesMap.value[chatId] = msgs
    } catch (e) {
      console.error('Failed to load history', e)
      messagesMap.value[chatId] = []
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
    loadHistory,
    addMessage,
    addChat,
    setActiveChat,
  }
})