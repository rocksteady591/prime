import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { useAuthStore } from './auth'
import { apiFetch } from '@/utils/api'
import { getOutbox, removeFromOutbox } from '@/utils/outbox'
import type { Message } from '@/types/message'

type ChatListItem = {
  id: string
  name: string
  unread?: number
  raw?: { id: number; user1_id: number; user2_id: number; [k: string]: any }
}

export const useChatsStore = defineStore('chats', () => {
  const auth = useAuthStore()
  const chatList = ref<ChatListItem[]>([])
  const activeChatId = ref<string | null>(null)
  const messagesMap = ref<Record<string, Message[]>>({})
  const loadingHistory = ref<Record<string, boolean>>({})

  // чаты, для которых loadHistory вызвали ДО того, как chatList был готов
  const pendingHistory = ref<Set<string>>(new Set())

  // ==========================================================
  //  Загрузка истории
  //  - append=true  подгрузка вверх (пагинация)
  //  - append=false  полная замена + сверка с outbox
  // ==========================================================
  async function loadHistory(
    chatId: string,
    limit = 50,
    offset = 0,
    append = false,          // ← вот он, параметр
  ) {
    const chat = chatList.value.find(c => c.id === chatId)
    if (!chat) {
      // chatList ещё не пришёл — запомним и загрузим после loadChats()
      pendingHistory.value.add(chatId)
      return
    }
    pendingHistory.value.delete(chatId)

    const realChatId = chat.raw?.id
    if (realChatId == null) {
      console.warn('No raw chat id for', chatId)
      return
    }

    loadingHistory.value[chatId] = true
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
      })

      if (!res.ok) {
        throw new Error(res.data?.error || res.error || 'Failed to load messages')
      }

      const data = res.data || []
      const msgs: Message[] = data.map((m: any) => ({
        id: m.msg_id?.toString(),
        sender: m.sender_id.toString(),
        text: m.text,
        timestamp: new Date(m.send_time).getTime(),
        status: m.sender_id.toString() === auth.userId ? 'delivered' : undefined,
      }))

      if (append) {
        messagesMap.value[chatId] = [
          ...msgs,
          ...(messagesMap.value[chatId] || []),
        ]
        return
      }

      // ---------- Полная замена + сверка с outbox ----------
      const outbox = getOutbox()[chatId] || []
      const serverKey = (m: Message) => `${m.sender}::${m.text}`
      const serverSet = new Set(msgs.map(serverKey))
      const extraFromOutbox: Message[] = []

      for (const pending of outbox) {
        if (serverSet.has(serverKey(pending))) {
          removeFromOutbox(chatId, pending.id!)
        } else {
          // сервер не знает → показываем как error
          extraFromOutbox.push({ ...pending, status: 'error' })
        }
      }

      messagesMap.value[chatId] = [...msgs, ...extraFromOutbox]
    } catch (e) {
      console.error('Failed to load history', e)

      //    вот здесь используется append — он в области видимости,
      //    потому что это параметр этой же функции loadHistory
      if (!append) {
        const outbox = getOutbox()[chatId] || []
        messagesMap.value[chatId] = outbox.map(m => ({
          ...m,
          status: 'error',              //  принудительно error при падении сети
        }))
      }
    } finally {
      loadingHistory.value[chatId] = false
    }
  }

  function updateMessageStatus(
    chatId: string,
    msgId: string,
    status: Message['status'],
  ) {
    const msgs = messagesMap.value[chatId]
    if (!msgs) return
    const idx = msgs.findIndex(m => m.id === msgId)
    if (idx !== -1) {
      msgs[idx] = { ...msgs[idx], status }
      messagesMap.value[chatId] = [...msgs]
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
        throw new Error(res.data?.message || res.error || 'Failed to load chats')
      }

      const data = res.data || []
      const fromServer: ChatListItem[] = data.map((c: any) => {
        const otherId = c.user1_id === userId ? c.user2_id : c.user1_id
        return {
          id: `chat_${Math.min(c.user1_id, c.user2_id)}_${Math.max(c.user1_id, c.user2_id)}`,
          name: `User ${otherId}`,
          raw: c,
        }
      })

      const localOnly = chatList.value.filter(
        c => !fromServer.find(s => s.id === c.id),
      )
      chatList.value = [...fromServer, ...localOnly]

      // Догружаем историю для тех, кто вызывал loadHistory до готовности chatList
      for (const id of Array.from(pendingHistory.value)) {
        if (chatList.value.find(c => c.id === id)) {
          pendingHistory.value.delete(id)
          loadHistory(id).catch(console.error)
        }
      }
    } catch (e) {
      console.error('Failed to load chats', e)
    }
  }

  function addMessage(chatId: string, msg: Message) {
    if (!messagesMap.value[chatId]) messagesMap.value[chatId] = []
    messagesMap.value[chatId].push(msg)
    messagesMap.value[chatId] = [...messagesMap.value[chatId]]
  }

  function addChat(chatId: string, name: string, raw?: ChatListItem['raw']) {
    const existing = chatList.value.find(c => c.id === chatId)
    if (existing) {
      if (raw) existing.raw = raw
      return
    }
    chatList.value.push({ id: chatId, name, raw })
  }

  function setActiveChat(id: string) {
    activeChatId.value = id
    if (!messagesMap.value[id] || messagesMap.value[id].length === 0) {
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