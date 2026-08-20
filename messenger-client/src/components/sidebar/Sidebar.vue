<template>
    <aside class="sidebar">
        <!-- Шапка -->
        <div class="sidebar-header">
            <div class="brand">
                <i class="pi pi-comment" style="font-size: 1.5rem; color: var(--primary-color);" />
                <span>{{ title }}</span>
            </div>
            <Button icon="pi pi-times" text rounded severity="secondary" @click="$emit('close')" class="close-btn" />
        </div>

        <!-- Поле добавления чата -->
        <div class="add-chat">
            <InputGroup>
                <InputText v-model="newChatLogin" placeholder="Введите логин собеседника" />
                <Button icon="pi pi-plus" severity="primary" @click="addChat" :loading="adding" />
            </InputGroup>
        </div>

        <!-- Список чатов -->
        <div class="chat-list">
            <div v-for="item in items ?? []"
                :key="item.id"
                class="chat-item"
                :class="{ active: item.id === activeId }"
                @click="$emit('select', item.id)">
                <Avatar icon="pi pi-user" size="normal" shape="circle" class="chat-avatar" />
                <div class="chat-info">
                    <span class="chat-name">{{ item.name }}</span>
                    <span v-if="item.lastMessage" class="last-message">{{ item.lastMessage }}</span>
                </div>
                <span v-if="item.unread" class="unread-badge">{{ item.unread }}</span>
            </div>
            <div v-if="(items ?? []).length === 0" class="empty-list">
                <i class="pi pi-inbox" style="font-size: 2rem; color: var(--text-color-secondary);" />
                <p>Нет активных чатов</p>
            </div>
        </div>

        <!-- ID пользователя -->
        <div class="my-id" v-if="userId">
            <i class="pi pi-user" style="margin-right: 6px;" />
            {{ userId }}
        </div>
    </aside>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import Button from 'primevue/button'
import InputText from 'primevue/inputtext'
import InputGroup from 'primevue/inputgroup'
import Avatar from 'primevue/avatar'
import { useAuthStore } from '@/stores/auth'
import { useToast } from 'primevue/usetoast'

const props = defineProps<{
    title: string
    items?: Array<{ id: string; name: string; lastMessage?: string; unread?: number }>
    activeId: string | null
    userId?: string
}>()

const emit = defineEmits<{
    close: []
    select: [id: string]
    add: [id: string, name: string]
}>()

const auth = useAuthStore()
const toast = useToast()

const newChatLogin = ref('')
const adding = ref(false)

async function addChat() {
    const login = newChatLogin.value.trim()
    if (!login) {
        toast.add({ severity: 'warn', summary: 'Ошибка', detail: 'Введите логин', life: 3000 })
        return
    }

    adding.value = true
    try {
        const response = await fetch('/api/find_user', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                Authorization: `Bearer ${auth.token}`,
            },
            body: JSON.stringify({ login }),
        })
        const data = await response.json()
        if (!response.ok) {
            throw new Error(data.message || 'Пользователь не найден')
        }

        const userId = data.user_id.toString()
        const username = data.username || data.user_login

        // Проверка на самого себя
        if (userId === auth.userId) {
            toast.add({ severity: 'error', summary: 'Ошибка', detail: 'Нельзя добавить себя в чат', life: 3000 })
            return
        }

        // Проверяем, есть ли уже такой чат
        const existing = props.items?.find(c => c.id === `chat_${Math.min(parseInt(userId), parseInt(auth.userId || '0'))}_${Math.max(parseInt(userId), parseInt(auth.userId || '0'))}`)
        if (existing) {
            toast.add({ severity: 'info', summary: 'Уже есть', detail: 'Чат с этим пользователем уже существует', life: 3000 })
            emit('select', existing.id)
            newChatLogin.value = ''
            return
        }

        // Создаём чат
        emit('add', userId, username)
        newChatLogin.value = ''
        toast.add({ severity: 'success', summary: 'Готово', detail: `Чат с ${username} создан`, life: 3000 })
    } catch (e: any) {
        toast.add({ severity: 'error', summary: 'Ошибка', detail: e.message || 'Не удалось найти пользователя', life: 5000 })
        console.error('Ошибка при добавлении чата:', e)
    } finally {
        adding.value = false
    }
}
</script>

<style scoped>
    .sidebar {
        width: 320px;
        min-width: 320px;
        background: var(--surface-0);
        display: flex;
        flex-direction: column;
        height: 100%;
        border-right: 1px solid var(--surface-border);
        position: relative;
    }

    .sidebar-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 1.25rem 1.5rem;
        border-bottom: 1px solid var(--surface-border);
    }

    .brand {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        font-weight: 600;
        font-size: 1.1rem;
        color: var(--text-color);
    }

    .close-btn {
        display: none;
    }

    .add-chat {
        padding: 1rem 1.5rem;
        background: var(--surface-ground);
        border-bottom: 1px solid var(--surface-border);
    }

    .chat-list {
        flex: 1;
        overflow-y: auto;
        padding: 0.5rem 0.75rem;
    }

    .chat-item {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        padding: 0.75rem 1rem;
        border-radius: var(--border-radius);
        cursor: pointer;
        transition: background 0.15s, transform 0.1s;
        margin-bottom: 2px;
    }

        .chat-item:hover {
            background: var(--surface-hover);
            transform: translateX(4px);
        }

        .chat-item.active {
            background: var(--primary-50);
            color: var(--primary-color);
        }

    .chat-avatar {
        flex-shrink: 0;
        background: var(--primary-color);
        color: white;
    }

    .chat-info {
        flex: 1;
        min-width: 0;
    }

    .chat-name {
        font-weight: 500;
        display: block;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    .last-message {
        font-size: 0.8rem;
        color: var(--text-color-secondary);
        display: block;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
    }

    .unread-badge {
        background: var(--primary-color);
        color: white;
        border-radius: var(--border-radius-circle);
        padding: 0.1rem 0.6rem;
        font-size: 0.7rem;
        font-weight: 600;
        min-width: 20px;
        text-align: center;
    }

    .empty-list {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100%;
        color: var(--text-color-secondary);
        gap: 0.5rem;
    }

    .my-id {
        padding: 0.75rem 1.5rem;
        font-size: 0.8rem;
        color: var(--text-color-secondary);
        border-top: 1px solid var(--surface-border);
        background: var(--surface-ground);
        display: flex;
        align-items: center;
    }

    /* Адаптивность */
    @media (max-width: 768px) {
        .sidebar {
            width: 100%;
            min-width: unset;
            position: fixed;
            top: 0;
            left: 0;
            height: 100%;
            z-index: 1000;
            transform: translateX(-100%);
            transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            box-shadow: var(--shadow-lg);
        }

            .sidebar.open {
                transform: translateX(0);
            }

        .close-btn {
            display: block;
        }
    }
</style>