<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="Чаты"
                     :items="store.chatList"
                     :activeId="store.activeChatId"
                     :userId="auth.userId || undefined"
                     @select="store.setActiveChat"
                     @add="onAddChat" />
        </template>

        <template #main>
            <div v-if="wsStore.status !== 'connected'" class="connecting">
                <ProgressSpinner />
                <span>{{ wsStore.status === 'connecting' ? 'Подключение...' : 'Ошибка подключения' }}</span>
            </div>

            <ChatWindow v-else-if="store.activeChatId"
                        :key="store.activeChatId"
                        :messages="store.activeMessages"
                        :currentUserId="auth.userId || ''"
                        :chatName="store.activeChatName"
                        @send="onSend" />

            <div v-else class="no-chat">
                Выберите чат или создайте новый
            </div>
        </template>
    </MainLayout>
</template>

<script setup lang="ts">
    import { onMounted } from 'vue'
    import ProgressSpinner from 'primevue/progressspinner'
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'
    import ChatWindow from '@/components/chat/ChatWindow.vue'
    import { useChatsStore } from '@/stores/chat'
    import { useAuthStore } from '@/stores/auth'
    import { useWebSocketStore } from '@/stores/websocket'

    const store = useChatsStore()
    const auth = useAuthStore()
    const wsStore = useWebSocketStore()

    onMounted(async () => {
        if (auth.userId) {
            await store.loadChats(parseInt(auth.userId))
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
        const myId = auth.userId
        const ids = [parseInt(myId || '0'), parseInt(id)].sort((a, b) => a - b)
        const chatId = `chat_${ids[0]}_${ids[1]}`
        store.addChat(chatId, name)
        store.setActiveChat(chatId)
    }
</script>

<style scoped>
    .connecting {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 1rem;
        background: var(--surface-ground);
        color: var(--text-color-secondary);
    }
    .no-chat {
        flex: 1;
        display: flex;
        align-items: center;
        justify-content: center;
        color: var(--text-color-secondary);
        background: var(--surface-ground);
        font-size: 1.2rem;
    }
</style>