import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface ChatItem {
  id: string
  name: string
}

export const useChatStore = defineStore('chat', () => {
  const chats = ref<ChatItem[]>([
    { id: 'user-abc', name: 'Алиса' },
    { id: 'user-xyz', name: 'Боб' },
  ])
  const contacts = ref<ChatItem[]>([
    { id: 'user-abc', name: 'Алиса' },
    { id: 'user-xyz', name: 'Боб' },
    { id: 'user-123', name: 'Чарли' },
  ])
  const activeChatId = ref<string | null>(null)

  const activeChat = computed(() =>
    chats.value.find(c => c.id === activeChatId.value) || null
  )

  function setActiveChat(id: string) {
    activeChatId.value = id
  }

  return { chats, contacts, activeChatId, activeChat, setActiveChat }
})