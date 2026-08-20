<template>
    <div :class="['message-row', isOutgoing ? 'outgoing' : 'incoming']">
        <Avatar v-if="!isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
        <div :class="['bubble', isOutgoing ? 'outgoing' : 'incoming']">
            <div class="sender-name" v-if="!isOutgoing">{{ sender }}</div>
            <div class="text">{{ text }}</div>
            <div class="time">{{ formatTime(timestamp) }}</div>
        </div>
        <Avatar v-if="isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
    </div>
</template>

<script setup lang="ts">
    import Avatar from 'primevue/avatar'

    defineProps<{
    text: string
    sender: string
    isOutgoing: boolean
    timestamp: number
    }>()

    function formatTime(ts: number) {
    return new Date(ts).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    }
</script>

<style scoped>
    .message-row {
        display: flex;
        align-items: flex-end;
        gap: 0.5rem;
        max-width: 75%;
        animation: messageIn 0.2s ease-out;
    }

    .incoming {
        align-self: flex-start;
    }

    .outgoing {
        align-self: flex-end;
        flex-direction: row-reverse;
    }

    .avatar {
        flex-shrink: 0;
        margin-bottom: 4px;
        background: var(--primary-light);
        color: white;
    }

    .bubble {
        padding: 0.6rem 1rem;
        border-radius: 8px;
        background: #f0f0f0; /* временно */
        border: 1px solid #ccc; /* временно */
        color: #000;
        word-wrap: break-word;
    }

    .incoming .bubble {
        background: #fff;
        border: 1px solid #ddd;
    }

    .outgoing .bubble {
        background: #007bff;
        color: #fff;
    }

    .sender-name {
        font-weight: 600;
        font-size: 0.8rem;
        margin-bottom: 2px;
        color: var(--primary-dark);
    }

    .text {
        font-size: 0.95rem;
        line-height: 1.5;
    }

    .time {
        font-size: 0.65rem;
        opacity: 0.7;
        text-align: right;
        margin-top: 4px;
    }

    .outgoing .time {
        color: rgba(255,255,255,0.8);
    }

    /* Адаптив */
    @media (max-width: 480px) {
        .message-row {
            max-width: 90%;
        }

        .bubble {
            padding: 0.5rem 0.75rem;
        }
    }

    @keyframes messageIn {
        from {
            opacity: 0;
            transform: translateY(8px);
        }

        to {
            opacity: 1;
            transform: translateY(0);
        }
    }
</style>