<template>
  <button
    class="theme-toggle"
    :title="isDark ? 'Светлая тема' : 'Тёмная тема'"
    @click="toggle"
  >
    <i :class="isDark ? 'pi pi-sun' : 'pi pi-moon'" />
  </button>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

const isDark = ref(true)

function apply(t: 'dark' | 'light') {
  isDark.value = t === 'dark'
  document.documentElement.classList.toggle('dark-mode', t === 'dark')
  document.documentElement.classList.toggle('light-mode', t === 'light')
  localStorage.setItem('theme', t)
}

function toggle() {
  apply(isDark.value ? 'light' : 'dark')
}

onMounted(() => {
  const saved = localStorage.getItem('theme') as 'dark' | 'light' | null
  if (saved) {
    apply(saved)
  } else {
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches
    apply(prefersDark ? 'dark' : 'light')
  }
})
</script>

<style scoped>
.theme-toggle {
  width: 40px;
  height: 40px;
  display: grid;
  place-items: center;
  background: transparent;
  border: none;
  cursor: pointer;
  border-radius: var(--radius-full);
  color: var(--text-muted);
  transition: var(--transition-fast);
}
.theme-toggle:hover {
  background: var(--bg-hover);
  color: var(--accent);
  transform: rotate(15deg);
}
.theme-toggle i { font-size: 1.1rem; transition: transform var(--transition-base); }
</style>