<template>
    <div class="messages-container" ref="containerRef">
        <MessageBubble
            v-for="msg in messages"
            :key="msg.timestamp + msg.sender + msg.text"
            :text="msg.text"
            :sender="msg.sender"
            :isOutgoing="msg.sender === currentUserId"
            :timestamp="msg.timestamp"
            />
    </div>
</template>

<script setup lang="ts">
    import { ref, watch, nextTick } from 'vue'
    import MessageBubble from './MessageBubble.vue'
    import type { Message } from '@/types/message'

    const props = defineProps<{
        messages: Message[]
        currentUserId: string
    }>()

    const containerRef = ref<HTMLElement | null>(null)

    watch(() => props.messages.length, async () => {
        await nextTick()
        if (containerRef.value) {
            containerRef.value.scrollTop = containerRef.value.scrollHeight
        }
    }, { flush: 'post' })
</script>

<style scoped>
    .messages-container {
        flex: 1;
        padding: 20px;
        overflow-y: auto;
        display: flex;
        flex-direction: column;
        gap: 15px;
    }
</style>