<template>
  <div class="avatar-badge" :class="{ online }" :style="{ width: size + 'px', height: size + 'px' }">
    <div class="inner" :style="{ background: gradient, fontSize: size * 0.38 + 'px' }">
      {{ initials }}
    </div>
    <span v-if="online !== undefined" class="status-dot" :class="{ online }" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  name?: string
  id?: string | number
  size?: number
  online?: boolean
}>(), { size: 36 })

const initials = computed(() => {
  const s = (props.name || '?').trim()
  const parts = s.split(/\s+/)
  if (parts.length >= 2) return (parts[0][0] + parts[1][0]).toUpperCase()
  return s.slice(0, 2).toUpperCase()
})

// Стабильный градиент по id
const gradients = [
  ['#6366F1', '#8B5CF6'],   // indigo→violet
  ['#EC4899', '#F43F5E'],   // pink→rose
  ['#10B981', '#14B8A6'],   // emerald→teal
  ['#F59E0B', '#EF4444'],   // amber→red
  ['#06B6D4', '#3B82F6'],   // cyan→blue
  ['#8B5CF6', '#EC4899'],   // violet→pink
  ['#84CC16', '#22C55E'],   // lime→green
  ['#F97316', '#EAB308'],   // orange→yellow
]
const gradient = computed(() => {
  const seed = String(props.id ?? props.name ?? '0')
  let hash = 0
  for (let i = 0; i < seed.length; i++) hash = (hash * 31 + seed.charCodeAt(i)) >>> 0
  const [a, b] = gradients[hash % gradients.length]
  return `linear-gradient(135deg, ${a}, ${b})`
})
</script>

<style scoped>
.avatar-badge {
  position: relative;
  border-radius: 50%;
  flex-shrink: 0;
  transition: transform var(--transition-fast);
}
.avatar-badge.online { box-shadow: 0 0 0 2px var(--bg-secondary), 0 0 0 4px var(--success); }
.avatar-badge:hover { transform: scale(1.06); }
.inner {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  display: grid;
  place-items: center;
  color: white;
  font-weight: 600;
  letter-spacing: 0.5px;
  user-select: none;
}
.status-dot {
  position: absolute;
  right: -1px;
  bottom: -1px;
  width: 11px;
  height: 11px;
  border-radius: 50%;
  background: var(--text-faint);
  border: 2px solid var(--bg-secondary);
}
.status-dot.online { background: var(--success); box-shadow: 0 0 0 0 var(--success); animation: online-pulse 2s infinite; }
@keyframes online-pulse {
  0%   { box-shadow: 0 0 0 0 rgba(34, 197, 94, 0.6); }
  100% { box-shadow: 0 0 0 8px rgba(34, 197, 94, 0); }
}
</style>