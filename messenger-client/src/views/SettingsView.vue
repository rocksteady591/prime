<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="НАСТРОЙКИ"
                     :items="settingsItems"
                     :activeId="null"
                     @select="() => {}" />
        </template>
        <template #main>
            <div class="placeholder">Здесь будут настройки</div>
        </template>
    </MainLayout>
</template>

<script setup lang="ts">
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'

    const settingsItems = [
    { id: 'theme', name: 'Тема' },
    { id: 'notifications', name: 'Уведомления' },
    { id: 'security', name: 'Безопасность' },
    ]
</script>

<style scoped>
    .placeholder {
        flex: 1;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 18px;
        color: #888;
        background-color: #f9f9f9;
    }
</style>