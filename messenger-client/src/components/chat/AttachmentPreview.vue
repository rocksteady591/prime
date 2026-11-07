<template>
  <div class="attachment-bar">
    <div
      v-for="att in attachments"
      :key="att.id"
      class="att-chip"
    >
      <img
        v-if="att.preview"
        :src="att.preview"
        :alt="att.name"
        class="att-thumb"
      />
      <i v-else class="pi pi-file" />

      <div class="att-info">
        <span class="att-name">{{ att.name }}</span>
        <span class="att-size">{{ formatSize(att.size) }}</span>
      </div>

      <button class="att-remove" @click="$emit('remove', att.id)">
        <i class="pi pi-times" />
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import type { Attachment } from '@/types/message'
defineProps<{ attachments: Attachment[] }>()
defineEmits<{ (e: 'remove', id: string): void }>()

function formatSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} Б`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} КБ`
  return `${(bytes / (1024 * 1024)).toFixed(1)} МБ`
}
</script>

<style scoped>
.attachment-bar {
  display: flex;
  gap: 0.5rem;
  padding: 0.5rem 1.25rem 0;
  flex-wrap: wrap;
  background: var(--bg-secondary);
}
.att-chip {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.4rem 0.5rem 0.4rem 0.4rem;
  background: var(--bg-primary);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  max-width: 240px;
}
.att-thumb {
  width: 40px;
  height: 40px;
  border-radius: var(--radius-sm);
  object-fit: cover;
  flex-shrink: 0;
}
.att-chip > i {
  width: 40px;
  height: 40px;
  display: grid;
  place-items: center;
  background: var(--accent-soft);
  color: var(--accent);
  border-radius: var(--radius-sm);
  flex-shrink: 0;
}
.att-info {
  display: flex;
  flex-direction: column;
  min-width: 0;
}
.att-name {
  font-size: 0.8rem;
  color: var(--text-main);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.att-size {
  font-size: 0.7rem;
  color: var(--text-muted);
}
.att-remove {
  width: 24px;
  height: 24px;
  flex-shrink: 0;
  display: grid;
  place-items: center;
  background: transparent;
  border: none;
  cursor: pointer;
  border-radius: var(--radius-full);
  color: var(--text-muted);
  transition: var(--transition-fast);
}
.att-remove:hover { background: var(--danger-soft); color: var(--danger); }
.att-remove i { font-size: 0.7rem; }
</style>