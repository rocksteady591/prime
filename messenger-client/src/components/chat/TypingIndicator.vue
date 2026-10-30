<template>
  <div v-if="typing" class="typing-indicator">
    <Avatar icon="pi pi-user" shape="circle" class="avatar" />
    <div class="bubble">
      <span class="dot" /><span class="dot" /><span class="dot" />
    </div>
  </div>
</template>

<script setup lang="ts">
import Avatar from 'primevue/avatar'
defineProps<{ typing: boolean }>()
</script>

<style scoped>
.typing-indicator {
  display: flex; align-items: flex-end; gap: 0.5rem;
  align-self: flex-start; margin-top: 4px;
  animation: messageIn 0.25s var(--ease-out);
}
.avatar { background: var(--accent); color: white; }
.bubble {
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  padding: 0.6rem 1rem;
  border-radius: var(--radius-lg);
  border-bottom-left-radius: 4px;
  display: flex; gap: 4px; align-items: center;
}
.dot {
  width: 6px; height: 6px; border-radius: 50%;
  background: var(--text-muted);
  animation: blink 1.2s infinite;
}
.dot:nth-child(2) { animation-delay: 0.2s; }
.dot:nth-child(3) { animation-delay: 0.4s; }

@keyframes blink {
  0%, 60%, 100% { opacity: 0.3; transform: translateY(0); }
  30% { opacity: 1; transform: translateY(-3px); }
}
@keyframes messageIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }
</style>