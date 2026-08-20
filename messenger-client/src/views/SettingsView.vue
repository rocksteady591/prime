<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="Настройки" :items="settingsItems" :activeId="null" />
        </template>
        <template #main>
            <div class="placeholder-container">
                <i class="pi pi-cog" style="font-size: 4rem; color: var(--surface-400);" />
                <h2>Настройки</h2>
                <p>Здесь будут доступны настройки приложения</p>
            </div>
        </template>
    </MainLayout>
</template>

<script setup>
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'

    const settingsItems = [
    { id: 'theme', name: 'Тема' },
    { id: 'notifications', name: 'Уведомления' },
    { id: 'security', name: 'Безопасность' },
    ]
</script>

<style scoped>
    .placeholder-container {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        background: var(--surface-ground);
        color: var(--text-color-secondary);
        gap: 0.5rem;
    }

        .placeholder-container h2 {
            margin: 0.5rem 0 0;
            font-weight: 400;
        }
</style>