<template>
    <div id="app">
        <h1>Секретный чат</h1>
        <ChatWindow :messages="messages" />
        <MessageInput @send="sendMessage" />
        <p>Статус: {{ status }}</p>
        <button v-if="status === 'disconnected'" @click="connect()">Подключиться</button>
    </div>
</template>

<script setup>
    import { onMounted } from 'vue';
    import { useSecureChat } from './composables/useSecureChat.js';
    import ChatWindow from './components/ChatWindow.vue';
    import MessageInput from './components/MessageInput.vue';

    const { messages, status, connect, sendMessage } = useSecureChat();

    onMounted(() => {
    connect();
    });
</script>