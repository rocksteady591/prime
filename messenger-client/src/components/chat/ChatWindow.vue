<template>
    <main class="chat-window">
        <!-- Шапка чата -->
        <div class="chat-header">
            <Avatar icon="pi pi-user" size="large" shape="circle" />
            <h3>{{ chatName || 'Чат' }}</h3>
        </div>

        <!-- Сообщения -->
        <div class="messages-container" ref="messagesContainer">
            <MessageBubble v-for="msg in messages"
                           :key="msg.timestamp + msg.sender + msg.text"
                           :text="msg.text"
                           :sender="msg.sender"
                           :isOutgoing="msg.sender === currentUserId" />
        </div>

        <!-- Поле ввода -->
        <div class="input-panel">
            <InputText v-model="newMessage"
                       placeholder="Введите сообщение..."
                       @keyup.enter="onSend"
                       class="message-input" />
            <Button icon="pi pi-send"
                    severity="primary"
                    rounded
                    @click="onSend"
                    :disabled="!newMessage.trim()" />
        </div>
    </main>
</template>

<script setup lang="ts">
    import { ref, watch, nextTick } from 'vue'
    import Button from 'primevue/button'
    import InputText from 'primevue/inputtext'
    import Avatar from 'primevue/avatar'
    import MessageBubble from './MessageBubble.vue'
    import type { Message } from '@/composables/useSecureChat'

    const props = defineProps<{
    messages: Message[]
    currentUserId: string
    chatName?: string
    }>()

    const emit = defineEmits<{
    send: [text: string]
    }>()

    const newMessage = ref('')
    const messagesContainer = ref<HTMLElement | null>(null)

    function onSend() {
    const text = newMessage.value.trim()
    if (text) {
    emit('send', text)
    newMessage.value = ''
    }
    }

    // Автопрокрутка вниз при новых сообщениях
    watch(() => props.messages.length, () => {
    nextTick(() => {
    if (messagesContainer.value) {
    messagesContainer.value.scrollTop = messagesContainer.value.scrollHeight
    }
    })
    })
</script>

<style scoped>
    .chat-window {
        flex: 1;
        display: flex;
        flex-direction: column;
        background-color: #e5ddd5;
        position: relative;
    }

    .chat-header {
        display: flex;
        align-items: center;
        gap: 12px;
        padding: 12px 20px;
        background: #fff;
        border-bottom: 1px solid #e0e0e0;
        box-shadow: 0 2px 4px rgba(0,0,0,0.05);
    }

        .chat-header h3 {
            margin: 0;
            font-size: 18px;
        }

    .messages-container {
        flex: 1;
        padding: 20px;
        overflow-y: auto;
        display: flex;
        flex-direction: column;
        gap: 12px;
    }

    .input-panel {
        padding: 12px 20px;
        background: #f0f2f5;
        display: flex;
        align-items: center;
        gap: 10px;
    }

    .message-input {
        flex: 1;
    }
</style>