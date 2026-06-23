<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="АККАУНТ"
                     :items="accountItems"
                     :activeId="null"
                     @select="() => {}" />
        </template>
        <template #main>
            <div class="placeholder">Профиль пользователя</div>
        </template>
    </MainLayout>
</template>

<script setup lang="ts">
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'

    const accountItems = [
    { id: 'profile', name: 'Профиль' },
    { id: 'keys', name: 'Ключи шифрования' },
    { id: 'logout', name: 'Выйти' },
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