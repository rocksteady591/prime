<!-- src/App.vue -->
<template>
  <div class="app-shell">
    <router-view />
    <Toast />
  </div>
</template>

<script setup lang="ts">
import Toast from 'primevue/toast'
</script>

<style scoped>
.app-shell {
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background: var(--bg-primary);
  color: var(--text-main);
  font-family: var(--font-main);
}
</style>