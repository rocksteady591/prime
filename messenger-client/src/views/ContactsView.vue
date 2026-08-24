<template>
    <MainLayout>
        <!-- <template #sidebar>
            <Sidebar title="Контакты" :items="store.contacts" :activeId="null" />
        </template>
        <template #main>
            <div class="contacts-grid">
                <Card v-for="c in store.contacts" :key="c.id" class="contact-card">
                    <template #title>
                        <div class="contact-title">
                            <Avatar icon="pi pi-user" size="large" shape="circle" class="contact-avatar" />
                            {{ c.name }}
                        </div>
                    </template>
                    <template #content>
                        <p class="contact-id">ID: {{ c.id }}</p>
                    </template>
                    <template #footer>
                        <Button label="Написать" icon="pi pi-comment" severity="primary" @click="startChat(c)" />
                    </template>
                </Card>
            </div>
        </template> -->
    </MainLayout>
</template>

<script setup lang="ts">
    import MainLayout from '@/components/layout/MainLayout.vue'
    import Sidebar from '@/components/sidebar/Sidebar.vue'
    import Card from 'primevue/card'
    import Button from 'primevue/button'
    import Avatar from 'primevue/avatar'
    import { useChatsStore } from '@/stores/chat'
    import { useRouter } from 'vue-router'
    import { onMounted } from 'vue'


    const store = useChatsStore()
    const router = useRouter()

    onMounted(() => {
    //   store.loadContacts()
    })

    function startChat(contact: { id: string; name: string }) {
    if (!store.chatList.some(c => c.id === contact.id)) {
    store.addChat(contact.id, contact.name)
    }
    store.setActiveChat(contact.id)
    router.push('/chats')
    }
</script>

<style scoped>
    .contacts-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
        gap: 1.5rem;
        padding: 2rem;
        flex: 1;
        align-content: start;
    }

    .contact-card {
        background: var(--surface-0);
        border-radius: var(--border-radius);
        box-shadow: var(--shadow-sm);
        transition: transform 0.2s, box-shadow 0.2s;
        text-align: center;
        padding: 1.5rem 1rem;
    }

        .contact-card:hover {
            transform: translateY(-4px);
            box-shadow: var(--shadow-md);
        }

    .contact-title {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        justify-content: center;
    }

    .contact-avatar {
        background: var(--primary-color-light);
        color: white;
    }

    .contact-id {
        color: var(--text-color-secondary);
        font-size: 0.9rem;
        margin: 0.5rem 0 0;
    }

    @media (max-width: 600px) {
        .contacts-grid {
            grid-template-columns: 1fr 1fr;
            padding: 1rem;
        }
    }
</style>