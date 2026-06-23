<template>
    <aside class="sidebar">
        <div class="sidebar-header">
            <span>{{ title }}</span>
            <Button icon="pi pi-times" text rounded @click="$emit('close')" />
        </div>

        <!-- Поле добавления чата -->
        <div class="add-chat p-2">
            <InputGroup>
                <InputText v-model="newChatId" placeholder="ID собеседника" />
                <Button icon="pi pi-plus" severity="primary" @click="addChat" />
            </InputGroup>
        </div>

        <!-- Список чатов -->
        <div class="chat-list">
            <div v-for="item in items"
                 :key="item.id"
                 class="chat-item"
                 :class="{ active: item.id === activeId }"
                 @click="$emit('select', item.id)">
                <Avatar icon="pi pi-user" size="normal" shape="circle" />
                <span class="chat-name">{{ item.name }}</span>
            </div>
            <div v-if="items.length === 0" class="empty-list">
                Нет активных чатов
            </div>
        </div>

        <!-- ID пользователя -->
        <div class="my-id" v-if="userId">
            Мой ID: {{ userId }}
        </div>
    </aside>
</template>

<script setup lang="ts">
    import { ref } from 'vue'
    import Button from 'primevue/button'
    import InputText from 'primevue/inputtext'
    import InputGroup from 'primevue/inputgroup'
    import Avatar from 'primevue/avatar'

    defineProps<{
    title: string
    items: Array<{ id: string; name: string }>
    activeId: string | null
    userId?: string
    }>()

    const emit = defineEmits<{
    close: []
    select: [id: string]
    add: [id: string, name: string]
    }>()

    const newChatId = ref('')

    function addChat() {
    const id = newChatId.value.trim()
    if (id) {
    emit('add', id, id)   // пока имя = ID, позже можно запрашивать
    newChatId.value = ''
    }
    }
</script>

<style scoped>
    .sidebar {
        width: 280px;
        background: var(--surface-card);
        border-right: 1px solid var(--surface-border);
        display: flex;
        flex-direction: column;
        height: 100%;
    }

    .sidebar-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 1rem;
        border-bottom: 1px solid var(--surface-border);
        font-weight: 600;
    }

    .chat-list {
        flex: 1;
        overflow-y: auto;
        padding: 0.5rem;
    }

    .chat-item {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        padding: 0.75rem;
        border-radius: 0.5rem;
        cursor: pointer;
        transition: background 0.2s;
    }

        .chat-item:hover {
            background: var(--surface-hover);
        }

        .chat-item.active {
            background: var(--primary-100);
            color: var(--primary-color);
        }

    .chat-name {
        font-weight: 500;
    }

    .empty-list {
        text-align: center;
        padding: 2rem;
        color: var(--text-color-secondary);
    }

    .my-id {
        padding: 0.75rem;
        text-align: center;
        font-size: 0.8rem;
        color: var(--text-color-secondary);
        border-top: 1px solid var(--surface-border);
        background: var(--surface-ground);
    }

    .add-chat {
        background: var(--surface-ground);
        border-bottom: 1px solid var(--surface-border);
    }
</style>