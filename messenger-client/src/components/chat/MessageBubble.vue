<template>
  <div :class="['message-row', isOutgoing ? 'outgoing' : 'incoming']">
    <Avatar v-if="!isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
    <div :class="['bubble', isOutgoing ? 'outgoing' : 'incoming']">
      <div class="sender-name" v-if="!isOutgoing">{{ sender }}</div>
      <div class="text">{{ text }}</div>
      <div class="time">{{ formatTime(timestamp) }}</div>
      <span v-if="isOutgoing" class="status-indicator">
          <i v-if="status === 'pending'" class="pi pi-spin pi-spinner" />
          <i v-else-if="status === 'sent'" class="pi pi-check" />
          <i v-else-if="status === 'delivered'" class="pi pi-check-circle" />
          <i v-else-if="status === 'error'" class="pi pi-exclamation-circle" style="color: var(--danger);" />
      </span>
      <Button
          v-if="isOutgoing && status === 'error'"
          icon="pi pi-refresh"
          size="small"
          text
          severity="danger"
          @click="emit('resend')"
          class="resend-btn"
        />
    </div>
    <Avatar v-if="isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
  </div>
</template>

<script setup lang="ts">
import Avatar from 'primevue/avatar'
import Button from 'primevue/button'

const props = defineProps<{
  text: string
  sender: string
  isOutgoing: boolean
  timestamp: number
  status?: 'pending' | 'sent' | 'delivered' | 'error'
}>()

const emit = defineEmits<{
  (e: 'resend'): void
}>()

function formatTime(ts: number) {
  return new Date(ts).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
}
</script>

<style scoped>
.footer {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 6px;
  margin-top: 4px;
}
.time {
  font-size: 0.65rem;
  opacity: 0.7;
}
.status-indicator {
  font-size: 0.7rem;
  color: var(--text-muted);
}
.outgoing .status-indicator {
  color: rgba(255,255,255,0.8);
}
.resend-btn {
  padding: 0;
  font-size: 0.7rem;
}

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
  background: var(--accent);
  color: white;
}

.bubble {
  padding: 0.6rem 1rem;
  border-radius: var(--radius-lg);
  word-wrap: break-word;
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  color: var(--text-main);
}

.incoming .bubble {
  border-bottom-left-radius: 4px;
}

.outgoing .bubble {
  background: var(--accent);
  color: white;
  border-color: var(--accent);
  border-bottom-right-radius: 4px;
}

.sender-name {
  font-weight: 600;
  font-size: 0.8rem;
  margin-bottom: 2px;
  color: var(--accent);
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
  color: rgba(255, 255, 255, 0.8);
}

@media (max-width: 480px) {
  .message-row {
    max-width: 90%;
  }
  .bubble {
    padding: 0.5rem 0.75rem;
  }
}

@keyframes messageIn {
  from { opacity: 0; transform: translateY(8px); }
  to { opacity: 1; transform: translateY(0); }
}
</style>