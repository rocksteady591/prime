<template>
    <MainLayout>
        <template #sidebar>
            <Sidebar title="КОНТАКТЫ" :items="store.contacts" :activeId="null" :showAdd="false" />
        </template>
        <template #main>
            <div class="contacts-grid">
                <Card v-for="c in store.contacts" :key="c.id" class="contact-card">
                    <template #title>
                        {{ c.name }}
                    </template>
                    <template #content>
                        <p>ID: {{ c.id }}</p>
                    </template>
                    <template #footer>
                        <Button label="Написать" icon="pi pi-comment" @click="startChat(c)" />
                    </template>
                </Card>
            </div>
        </template>
    </MainLayout>
</template>

<script setup lang="ts">
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'
    import Card from 'primevue/card'
    import Button from 'primevue/button'
    import { useChatStore } from '@/stores/chat'
    import { useRouter } from 'vue-router'

    const store = useChatStore()
    const router = useRouter()

    function startChat(contact: { id: string; name: string }) {
    if (!store.chats.some(c => c.id === contact.id)) {
    store.chats.push({ id: contact.id, name: contact.name })
    }
    store.setActiveChat(contact.id)
    router.push('/chats')
    }
</script>