<template>
  <div class="main-layout">
    <VerticalNav />
    <slot name="sidebar" />
    <slot name="main" />
  </div>
</template>

<script setup lang="ts">
import VerticalNav from './VerticalNav.vue'
</script>

<style scoped>
.main-layout {
  display: flex;
  flex: 1;
  height: 100vh;
  overflow: hidden;
  background: var(--bg-primary);
}
</style>