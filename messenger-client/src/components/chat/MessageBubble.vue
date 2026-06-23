<template>
    <div :class="['message-row', isOutgoing ? 'outgoing' : 'incoming']">
        <Avatar v-if="!isOutgoing"
                icon="pi pi-user"
                size="normal"
                shape="circle"
                class="avatar" />
        <div :class="['bubble', isOutgoing ? 'outgoing' : 'incoming']">
            <div class="sender-name" v-if="!isOutgoing">{{ sender }}</div>
            <div class="text">{{ text }}</div>
            <div class="time">{{ formatTime(timestamp) }}</div>
        </div>
        <Avatar v-if="isOutgoing"
                icon="pi pi-user"
                size="normal"
                shape="circle"
                class="avatar" />
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
        gap: 8px;
        max-width: 70%;
    }

    .incoming {
        align-self: flex-start;
    }

    .outgoing {
        align-self: flex-end;
        flex-direction: row-reverse;
    }

    .avatar {
        margin-bottom: 4px;
    }

    .bubble {
        padding: 10px 14px;
        border-radius: 18px;
        box-shadow: 0 1px 2px rgba(0,0,0,0.1);
        word-wrap: break-word;
        position: relative;
    }

    .incoming .bubble {
        background: #ffffff;
        border-top-left-radius: 4px;
    }

    .outgoing .bubble {
        background: #d9fdd3;
        border-top-right-radius: 4px;
    }

    .sender-name {
        font-weight: 600;
        font-size: 0.8rem;
        margin-bottom: 2px;
        color: #007bff;
    }

    .text {
        font-size: 0.95rem;
        line-height: 1.4;
    }

    .time {
        font-size: 0.7rem;
        color: #888;
        text-align: right;
        margin-top: 4px;
    }
</style>