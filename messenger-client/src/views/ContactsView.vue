<template>
  <MainLayout>
    <template #sidebar>
      <aside class="contacts-sidebar">
        <div class="sidebar-header">
          <h2>Контакты</h2>
        </div>

        <div class="search-block">
          <InputGroup>
            <InputText
              v-model="query"
              placeholder="Логин или имя пользователя…"
              @keyup.enter="search"
            />
            <Button icon="pi pi-search" severity="primary" @click="search" :loading="searching" />
          </InputGroup>
        </div>

        <div v-if="results.length" class="results">
          <div class="results-title">Найдены пользователи</div>
          <div v-for="r in results" :key="r.user_id" class="result-item">
            <Avatar icon="pi pi-user" shape="circle" />
            <div class="result-info">
              <span class="result-name">{{ r.username || r.user_login }}</span>
              <span class="result-id text-muted text-sm">ID: {{ r.user_id }}</span>
            </div>
            <Button
              icon="pi pi-comment"
              text
              rounded
              severity="primary"
              title="Написать"
              @click="startChat(r)"
            />
          </div>
        </div>

        <div class="section-title">Мои контакты</div>
        <div class="chat-list">
          <div
            v-for="c in contacts"
            :key="c.id"
            class="chat-item"
            @click="openChat(c)"
          >
            <Avatar icon="pi pi-user" shape="circle" class="chat-avatar" />
            <div class="chat-info">
              <span class="chat-name">{{ c.name }}</span>
            </div>
            <i class="pi pi-chevron-right chevron" />
          </div>
          <div v-if="!contacts.length" class="empty-list">
            <i class="pi pi-users" />
            <p>Пока нет контактов</p>
            <p class="text-muted text-sm">Найдите пользователя по логину выше</p>
          </div>
        </div>
      </aside>
    </template>

    <template #main>
      <div class="contacts-main">
        <div class="empty-state">
          <i class="pi pi-users" />
          <h3>Выберите контакт</h3>
          <p>Откройте существующий чат слева или найдите нового пользователя</p>
        </div>
      </div>
    </template>
  </MainLayout>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useRouter } from 'vue-router'
import Avatar from 'primevue/avatar'
import Button from 'primevue/button'
import InputText from 'primevue/inputtext'
import InputGroup from 'primevue/inputgroup'
import { useToast } from 'primevue/usetoast'
import MainLayout from '@/components/layout/MainLayout.vue'
import { useChatsStore } from '@/stores/chat'
import { useAuthStore } from '@/stores/auth'
import { apiFetch } from '@/utils/api'
import { getChatId } from '@/utils/chat'
import type { FindUserResponse } from '@/types/api'

const store = useChatsStore()
const auth = useAuthStore()
const router = useRouter()
const toast = useToast()

const query = ref('')
const searching = ref(false)
const results = ref<FindUserResponse[]>([])

const contacts = computed(() => store.chatList)

async function search() {
  const login = query.value.trim()
  if (!login) return
  searching.value = true
  results.value = []
  try {
    const res = await apiFetch<FindUserResponse>('/api/find_user', {
      method: 'POST',
      body: JSON.stringify({ user_name: login }),
    })
    if (!res.ok || !res.data) {
      toast.add({ severity: 'warn', summary: 'Не найдено', detail: res.error || 'Пользователь не найден', life: 3000 })
      return
    }
    const data = res.data
    if (String(data.user_id) === auth.userId) {
      toast.add({ severity: 'info', summary: 'Это вы', detail: 'Нельзя добавить себя', life: 2500 })
      return
    }
    results.value = [data]
  } catch (e: any) {
    toast.add({ severity: 'error', summary: 'Ошибка', detail: e.message, life: 4000 })
  } finally {
    searching.value = false
  }
}

function startChat(user: FindUserResponse) {
  if (!auth.userId) return
  const chatId = getChatId(auth.userId, user.user_id)
  store.addChat(chatId, user.username || user.user_login)
  store.setActiveChat(chatId)
  router.push('/chats')
}

function openChat(c: { id: string }) {
  store.setActiveChat(c.id)
  router.push('/chats')
}
</script>

<style scoped>
.contacts-sidebar {
  width: var(--sidebar-width);
  min-width: var(--sidebar-width);
  background: var(--bg-secondary);
  border-right: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  height: 100%;
}

.sidebar-header {
  padding: 1.25rem 1.5rem;
  border-bottom: 1px solid var(--border-color);
}
.sidebar-header h2 {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 600;
  color: var(--text-main);
}

.search-block {
  padding: 1rem 1.5rem;
  border-bottom: 1px solid var(--border-color);
  background: var(--bg-primary);
}
.search-block :deep(.p-inputgroup) {
  background: var(--bg-secondary);
  border-radius: var(--radius-full);
  overflow: hidden;
  border: 1px solid transparent;
  transition: var(--transition-fast);
}
.search-block :deep(.p-inputgroup:focus-within) {
  border-color: var(--accent);
  box-shadow: 0 0 0 3px var(--accent-glow);
}
.search-block :deep(.p-inputtext) {
  background: transparent !important;
  border: none !important;
  color: var(--text-main);
  padding: 0.6rem 1rem;
}
.search-block :deep(.p-button) {
  background: transparent !important;
  border: none !important;
  color: var(--accent);
}

.results {
  padding: 0.5rem 0.75rem;
  border-bottom: 1px solid var(--border-color);
}
.results-title,
.section-title {
  font-size: 0.7rem;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: var(--text-muted);
  font-weight: 600;
  padding: 0.5rem 0.75rem;
}
.result-item {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.5rem 0.75rem;
  border-radius: var(--radius-sm);
  transition: var(--transition-fast);
}
.result-item:hover { background: var(--bg-hover); }
.result-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}
.result-name {
  font-weight: 500;
  color: var(--text-main);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.section-title { padding: 0.75rem 1.5rem 0.25rem; }

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
  border-radius: var(--radius-sm);
  cursor: pointer;
  transition: var(--transition-fast);
  margin-bottom: 2px;
}
.chat-item:hover { background: var(--bg-hover); }
.chat-avatar {
  flex-shrink: 0;
  background: var(--accent);
  color: white;
}
.chat-info { flex: 1; min-width: 0; }
.chat-name {
  font-weight: 500;
  color: var(--text-main);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.chevron { color: var(--text-muted); }

.empty-list {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 2rem 1rem;
  color: var(--text-muted);
  gap: 0.5rem;
  text-align: center;
}
.empty-list i { font-size: 2rem; opacity: 0.4; }
.empty-list p { margin: 0; }

.contacts-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: var(--bg-primary);
}

.empty-state {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 0.75rem;
  padding: 2rem;
  color: var(--text-muted);
  text-align: center;
}
.empty-state i { font-size: 3.5rem; color: var(--accent); opacity: 0.4; }
.empty-state h3 { margin: 0; color: var(--text-main); font-size: 1.1rem; }
.empty-state p { margin: 0; }

@media (max-width: 768px) {
  .contacts-sidebar {
    width: 100%;
    min-width: unset;
  }
}
</style>