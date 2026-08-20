<template>
    <main class="chat-window">
        <!-- Шапка -->
        <div class="chat-header">
            <div class="chat-header-content">
                <Avatar icon="pi pi-user" size="large" shape="circle" class="header-avatar" />
                <div>
                    <h3>{{ chatName || 'Чат' }}</h3>
                </div>
            </div>
            <Button icon="pi pi-ellipsis-v" text rounded severity="secondary" />
        </div>

        <!-- Сообщения -->
        <div class="messages-container" ref="messagesContainer">
            <MessageBubble
                v-for="msg in messages"
                :key="msg.timestamp + msg.sender + msg.text"
                :text="msg.text"
                :sender="msg.sender"
                :isOutgoing="msg.sender === currentUserId"
                :timestamp="msg.timestamp"
            />
        </div>

        <!-- Поле ввода -->
        <div class="input-panel">
            <InputText
                v-model="newMessage"
                placeholder="Введите сообщение..."
                @keyup.enter="onSend"
                class="message-input"
            />
            <Button
                icon="pi pi-send"
                severity="primary"
                rounded
                @click="onSend"
                :disabled="!newMessage.trim()"
            />
        </div>
    </main>
</template>

<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import Button from 'primevue/button'
import InputText from 'primevue/inputtext'
import Avatar from 'primevue/avatar'
import MessageBubble from './MessageBubble.vue'
import type { Message } from '@/types/message'

const props = defineProps<{
    messages: Message[]
    currentUserId: string
    chatName?: string
}>()

const emit = defineEmits<{
    (e: 'send', text: string): void
}>()

const newMessage = ref('')
const messagesContainer = ref<HTMLElement | null>(null)

function onSend() {
    const text = newMessage.value.trim()
    if (!text) return
    emit('send', text)
    newMessage.value = ''
}

watch(
    () => props.messages.length,
    () => {
        nextTick(() => {
            if (messagesContainer.value) {
                messagesContainer.value.scrollTop = messagesContainer.value.scrollHeight
            }
        })
    }
)
</script>

<style scoped>
.chat-window {
    flex: 1;
    display: flex;
    flex-direction: column;
    height: 100vh;        /* или 100% – зависит от родителя */
    max-height: 100vh;
    background: var(--surface-50);
}

.chat-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.75rem 1.5rem;
    background: var(--surface-0);
    border-bottom: 1px solid var(--surface-200);
    box-shadow: var(--shadow-sm);
}

.chat-header-content {
    display: flex;
    align-items: center;
    gap: 0.75rem;
}

.header-avatar {
    background: var(--primary-light);
    color: white;
}

.chat-header h3 {
    margin: 0;
    font-size: 1.1rem;
    font-weight: 600;
}

.messages-container {
    flex: 1;
    overflow-y: auto;
    padding: 1.5rem 2rem;
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
    min-height: 0;
}

.input-panel {
    padding: 0.75rem 1.5rem;
    background: var(--surface-0);
    border-top: 1px solid var(--surface-200);
    display: flex;
    align-items: center;
    gap: 0.75rem;
}

.message-input {
    flex: 1;
    border-radius: var(--radius-full) !important;
    padding: 0.75rem 1.25rem !important;
    border: 1px solid var(--surface-300) !important;
    background: var(--surface-50) !important;
}

.message-input:focus {
    border-color: var(--primary) !important;
    box-shadow: 0 0 0 3px rgba(108, 92, 231, 0.15) !important;
}

@media (max-width: 768px) {
    .chat-header {
        padding: 0.5rem 1rem;
    }
    .messages-container {
        padding: 1rem;
    }
    .input-panel {
        padding: 0.5rem 1rem;
    }
}
</style>