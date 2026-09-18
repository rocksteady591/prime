<template>
  <button class="theme-toggle" @click="toggleTheme" :title="isDark ? 'Светлая тема' : 'Тёмная тема'">
    <i :class="isDark ? 'pi pi-sun' : 'pi pi-moon'" />
  </button>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from 'vue';

const isDark = ref(false);

// При монтировании читаем localStorage
onMounted(() => {
  const stored = localStorage.getItem('theme');
  if (stored === 'dark') {
    isDark.value = true;
    document.documentElement.classList.add('dark-mode');
  } else if (stored === 'light') {
    isDark.value = false;
    document.documentElement.classList.remove('dark-mode');
  } else {
    // Если нет сохранённого, используем системные настройки
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    isDark.value = prefersDark;
    if (prefersDark) {
      document.documentElement.classList.add('dark-mode');
    }
  }
});

function toggleTheme() {
  isDark.value = !isDark.value;
  if (isDark.value) {
    document.documentElement.classList.add('dark-mode');
    localStorage.setItem('theme', 'dark');
  } else {
    document.documentElement.classList.remove('dark-mode');
    localStorage.setItem('theme', 'light');
  }
}
</script>

<style scoped>
.theme-toggle {
  background: none;
  border: none;
  color: var(--text-muted);
  font-size: 1.3rem;
  cursor: pointer;
  padding: 0.5rem;
  border-radius: var(--radius-full);
  transition: var(--transition-fast);
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
}
.theme-toggle:hover {
  background: var(--bg-hover);
  color: var(--text-main);
}
</style>