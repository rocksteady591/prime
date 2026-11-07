<template>
  <div class="empty-state">
    <div class="illustration">
      <i :class="icon" />
    </div>
    <h3>{{ title }}</h3>
    <p v-if="description">{{ description }}</p>
    <slot name="action" />
  </div>
</template>

<script setup lang="ts">
withDefaults(defineProps<{
  icon?: string
  title: string
  description?: string
}>(), { icon: 'pi pi-inbox' })
</script>

<style scoped>
.empty-state {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 0.75rem;
  padding: 3rem 1.5rem;
  text-align: center;
  animation: fadeUp 0.45s var(--ease-out);
}
.illustration {
  width: 96px;
  height: 96px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  margin-bottom: 0.5rem;
  background: var(--accent-soft);
  box-shadow: inset 0 0 40px var(--accent-glow);
}
.illustration i { font-size: 2.5rem; color: var(--accent); opacity: 0.9; }
.empty-state h3 { margin: 0; font-size: 1.1rem; color: var(--text-main); }
.empty-state p {
  margin: 0;
  color: var(--text-muted);
  font-size: 0.9rem;
  max-width: 320px;
  line-height: 1.5;
}
@keyframes fadeUp {
  from { opacity: 0; transform: translateY(12px); }
  to   { opacity: 1; transform: translateY(0); }
}
</style>