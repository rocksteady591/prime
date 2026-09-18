<template>
  <MainLayout>
    <template #sidebar>
      <Sidebar
        title="Чаты"
        :class="{ open: sidebarOpen }"
        :items="store.chatList"
        :activeId="store.activeChatId"
        :userId="auth.userId || undefined"
        @close="sidebarOpen = false"
        @select="onSelectChat"
        @add="onAddChat"
      />
    </template>

    <template #main>
      <Button icon="pi pi-bars" text @click="sidebarOpen = !sidebarOpen" class="menu-toggle" />

      <div v-if="wsStore.status !== 'connected'" class="connecting">
        <ProgressSpinner />
        <span>{{ wsStore.status === 'connecting' ? 'Подключение...' : 'Ошибка подключения' }}</span>
      </div>

      <ChatWindow
        v-else-if="store.activeChatId && chat"
        :key="store.activeChatId"
        :messages="chat.messages.value"
        :currentUserId="auth.userId || ''"
        :chatName="chat.chatName.value"
        :onResendMessage="chat.resendMessage"
        @send="chat.sendMessage"
        @loadMore="chat.loadMore"
        @resend="chat.resendMessage"
      />

      <div v-else class="no-chat">Выберите чат или создайте новый</div>
    </template>
  </MainLayout>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import ProgressSpinner from 'primevue/progressspinner'
import Button from 'primevue/button'
import MainLayout from '@/components/layout/MainLayout.vue'
import Sidebar from '@/components/sidebar/Sidebar.vue'
import ChatWindow from '@/components/chat/ChatWindow.vue'
import { useChatsStore } from '@/stores/chat'
import { useAuthStore } from '@/stores/auth'
import { useWebSocketStore } from '@/stores/websocket'
import { useChat } from '@/composables/useChat'
import { getChatId } from '@/utils/chat'

const store = useChatsStore()
const auth = useAuthStore()
const wsStore = useWebSocketStore()

const sidebarOpen = ref(false)
const activeChatId = computed(() => store.activeChatId)
const chat = useChat(activeChatId)

function onSelectChat(id: string) {
  store.setActiveChat(id)
  sidebarOpen.value = false
}

function onAddChat(id: string, name: string) {
  const myId = auth.userId
  if (!myId) return
  const chatId = getChatId(myId, id)
  store.addChat(chatId, name)
  store.setActiveChat(chatId)
}

onMounted(async () => {
  if (auth.userId) {
    await store.loadChats()
    if (wsStore.status === 'disconnected') {
      wsStore.connect()
    }
  }
})
</script>

<style scoped>
.menu-toggle {
  display: none;
  position: fixed;
  top: 0.75rem;
  left: 1rem;
  z-index: 1100;
}
@media (max-width: 768px) {
  .menu-toggle {
    display: block;
  }
}
.connecting {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  color: var(--text-muted);
}
.no-chat {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-muted);
  font-size: 1.1rem;
}
</style>