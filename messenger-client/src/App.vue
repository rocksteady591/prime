<template>
    <AppNavbar />
    <router-view />
    <Toast />
</template>

<script setup lang="ts">
    import AppNavbar from './components/layout/AppNavbar.vue'
    import Toast from 'primevue/toast'
</script>