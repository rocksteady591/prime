<template>
  <div :class="['message-row', isOutgoing ? 'outgoing' : 'incoming']" :data-msg-id="msgId">
    <Avatar v-if="!isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
    <div :class="['bubble', isOutgoing ? 'outgoing' : 'incoming']">
      <div class="sender-name" v-if="!isOutgoing">{{ sender }}</div>
      <div v-if="attachments?.length" class="att-grid" :class="{ single: attachments.length === 1 }">
        <template v-for="att in attachments" :key="att.id">
          <a
            v-if="att.preview"
            :href="att.url"
            target="_blank"
            rel="noopener"
            class="att-img"
          >
            <img :src="att.preview" :alt="att.name" />
          </a>
          <a
            v-else
            :href="att.url"
            target="_blank"
            rel="noopener"
            class="att-file"
          >
            <i class="pi pi-file" />
            <span>{{ att.name }}</span>
          </a>
        </template>
      </div>
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
          @click="emit('resend', msgId)"
          class="resend-btn"
        />
    </div>
    <Avatar v-if="isOutgoing" icon="pi pi-user" size="normal" shape="circle" class="avatar" />
  </div>
</template>

<script setup lang="ts">
import { Attachment } from '@/types/message';
import Avatar from 'primevue/avatar'
import Button from 'primevue/button'

const props = defineProps<{
  msgId: string
  text: string
  sender: string
  isOutgoing: boolean
  timestamp: number
  status?: 'pending' | 'sent' | 'delivered' | 'error'
  attachments?: readonly Attachment[]
}>()

const emit = defineEmits<{
  (e: 'resend', msgId: string): void
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

.att-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 4px;
  margin-bottom: 6px;
  max-width: 320px;
}
.att-grid.single { grid-template-columns: 1fr; }
.att-img {
  display: block;
  border-radius: var(--radius-sm);
  overflow: hidden;
  aspect-ratio: 1;
}
.att-img img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  transition: transform var(--transition-base);
}
.att-img:hover img { transform: scale(1.03); }
.att-file {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 10px;
  background: rgba(0, 0, 0, 0.15);
  border-radius: var(--radius-sm);
  color: inherit;
  text-decoration: none;
  font-size: 0.85rem;
}
.att-file i { font-size: 1rem; opacity: 0.7; }
.att-file span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
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