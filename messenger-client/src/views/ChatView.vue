<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="Чаты"
                     :class="{ open: sidebarOpen }"
                     :items="store.chatList"
                     :activeId="store.activeChatId"
                     :userId="auth.userId || undefined"
                     @close="sidebarOpen = false"
                     @select="onSelectChat"
                     @add="onAddChat" />
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
                @send="chat.sendMessage"
            />

            <div v-else class="no-chat">
                Выберите чат или создайте новый
            </div>
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
    import { useChat } from '@/composables/useChat';
    import { getChatId } from '@/utils/chat';

    const store = useChatsStore()
    const auth = useAuthStore()
    const wsStore = useWebSocketStore()

    const sidebarOpen = ref(false)

    const activeChatId = computed(() => store.activeChatId);
    const chat = computed(() => {
        if (!activeChatId.value) return null;
        return useChat(activeChatId.value);
        });
    
    function onSelectChat(id: string) {
        store.setActiveChat(id);
        sidebarOpen.value = false;
    }    

    onMounted(async () => {
        if (auth.userId) {
            await store.loadChats()
            // Подключаемся только если ещё не подключены
            if (wsStore.status === 'disconnected') {
                wsStore.connect()
            }
        }
    })

    function onSend(text: string) {
        console.log('onSend called, text:', text)
        console.log('activeChatId:', store.activeChatId)
        console.log('wsStore.status:', wsStore.status)

        if (!store.activeChatId) {
            console.warn('Нет активного чата')
            return
        }

        const parts = store.activeChatId.split('_')
        const myId = auth.userId || ''
        const recipientId = parts[1] === myId ? parts[2] : parts[1]
        console.log('recipientId:', recipientId)

        const tempMsg = { sender: myId, text, timestamp: Date.now() }
        store.addMessage(store.activeChatId, tempMsg)

        const sent = wsStore.sendMessage(recipientId, text)
        console.log('sendMessage result:', sent)
    }

    function onAddChat(id: string, name: string) {
        const myId = auth.userId;
        if (!myId) return;
        const chatId = getChatId(myId, id);
        store.addChat(chatId, name);
        store.setActiveChat(chatId);
    }
</script>

<style>
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
</style>