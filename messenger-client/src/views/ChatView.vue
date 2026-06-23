<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="ЧАТЫ"
                     :items="store.chats"
                     :activeId="store.activeChatId"
                     :userId="myUserId"
                     @select="store.setActiveChat"
                     @add="onAddChat" />
        </template>

        <template #main>
            <!-- Индикатор подключения -->
            <div v-if="status !== 'connected'" class="connecting">
                <ProgressSpinner />
                <span>Подключение к серверу...</span>
            </div>

            <!-- Окно чата -->
            <ChatWindow v-else-if="store.activeChatId"
                        :key="store.activeChatId"
                        :messages="filteredMessages"
                        :currentUserId="myUserId"
                        :chatName="store.activeChat?.name"
                        @send="onSend" />

            <!-- Нет выбранного чата -->
            <div v-else class="no-chat">
                Выберите чат или создайте новый
            </div>
        </template>
    </MainLayout>
</template>

<script setup lang="ts">
    import { computed, onMounted } from 'vue'
    import ProgressSpinner from 'primevue/progressspinner'  // <-- добавили импорт
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'
    import ChatWindow from '@/components/chat/ChatWindow.vue'
    import { useSecureChat } from '@/composables/useSecureChat'
    import { useChatStore } from '@/stores/chat'
    import { useAuthStore } from '@/stores/auth'

    const store = useChatStore()
    const auth = useAuthStore()

    // Теперь забираем status
    const { messages, sendMessage, connect, myUserId, status } = useSecureChat(auth.userId ?? 'unknown')

    onMounted(() => {
    connect(auth.token ?? undefined)
    })

    const filteredMessages = computed(() =>
    messages.value.filter(m =>
    m.sender === store.activeChatId ||
    m.recipient === store.activeChatId
    )
    )

    function onSend(text: string) {
    if (store.activeChatId) {
    sendMessage(text, store.activeChatId)
    }
    }

    function onAddChat(id: string, name: string) {
    if (!store.chats.some(c => c.id === id)) {
    store.chats.push({ id, name })
    }
    store.setActiveChat(id)
    }
</script>

<style scoped>
    .connecting {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        background: #e5ddd5;
        color: #555;
        font-size: 16px;
    }

        .connecting span {
            margin-top: 12px;
        }

    .no-chat {
        flex: 1;
        display: flex;
        align-items: center;
        justify-content: center;
        background: #e5ddd5;
        color: #888;
        font-size: 18px;
    }

    .my-id {
        padding: 10px;
        font-size: 12px;
        color: #555;
        background: #f0f0f0;
        border-top: 1px solid #ddd;
        text-align: center;
    }
</style>