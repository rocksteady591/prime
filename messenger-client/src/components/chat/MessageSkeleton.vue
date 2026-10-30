<template>
  <div class="skeleton-stack">
    <div v-for="i in count" :key="i" class="skeleton-row" :class="i % 3 === 0 ? 'right' : 'left'">
      <div class="sk-avatar" />
      <div class="sk-bubble" :style="{ width: 120 + (i * 37) % 180 + 'px' }" />
    </div>
  </div>
</template>

<script setup lang="ts">
withDefaults(defineProps<{ count?: number }>(), { count: 6 })
</script>

<style scoped>
.skeleton-stack { display: flex; flex-direction: column; gap: 12px; padding: 12px 0; }
.skeleton-row { display: flex; gap: 8px; align-items: flex-end; }
.skeleton-row.right { flex-direction: row-reverse; }
.sk-avatar {
  width: 32px; height: 32px; border-radius: 50%;
  background: linear-gradient(90deg, var(--bg-hover) 0%, var(--bg-active) 50%, var(--bg-hover) 100%);
  background-size: 200% 100%;
  animation: shimmer 1.5s infinite;
}
.sk-bubble {
  height: 40px;
  border-radius: var(--radius-lg);
  background: linear-gradient(90deg, var(--bg-hover) 0%, var(--bg-active) 50%, var(--bg-hover) 100%);
  background-size: 200% 100%;
  animation: shimmer 1.5s infinite;
}
@keyframes shimmer {
  from { background-position: 200% 0; }
  to   { background-position: -200% 0; }
}
</style>