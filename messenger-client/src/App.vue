<template>
  <div class="app-layout">
    <router-view />
    <Toast />
  </div>
</template>

<script setup lang="ts">
import AppNavbar from './components/layout/AppNavbar.vue'
import Toast from 'primevue/toast'
</script>