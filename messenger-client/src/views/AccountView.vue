<template>
  <MainLayout>
    <template #sidebar>
      <aside class="account-sidebar">
        <div class="profile-card">
          <Avatar icon="pi pi-user" size="xlarge" shape="circle" class="profile-avatar" />
          <h3>{{ displayName }}</h3>
          <span class="text-muted text-sm">ID: {{ auth.userId }}</span>
        </div>
        <div class="settings-nav">
          <button
            v-for="s in sections"
            :key="s.id"
            class="nav-row"
            :class="{ active: active === s.id }"
            @click="active = s.id"
          >
            <i :class="s.icon" />
            <span>{{ s.label }}</span>
          </button>
          <button class="nav-row logout" @click="handleLogout">
            <i class="pi pi-sign-out" />
            <span>Выйти</span>
          </button>
        </div>
      </aside>
    </template>

    <template #main>
      <div class="account-main">
        <section v-if="active === 'profile'" class="account-section">
          <h2>Профиль</h2>
          <p class="section-desc">Основная информация об аккаунте</p>
          <div class="field">
            <label>Логин</label>
            <div class="field-value">{{ displayName }}</div>
          </div>
          <div class="field">
            <label>ID пользователя</label>
            <div class="field-value">
              <code>{{ auth.userId }}</code>
              <Button
                icon="pi pi-copy"
                text
                rounded
                severity="secondary"
                @click="copy(auth.userId || '')"
              />
            </div>
          </div>
          <div class="field">
            <label>Статус соединения</label>
            <div class="field-value">
              <span class="badge" :class="wsBadgeClass">{{ wsLabel }}</span>
            </div>
          </div>
        </section>

        <section v-if="active === 'keys'" class="account-section">
          <h2>Ключи шифрования</h2>
          <p class="section-desc">
            Сквозное шифрование (X25519 + XSalsa20-Poly1305).
            Ключи хранятся только в памяти вкладки и генерируются заново при каждом входе.
          </p>
          <div class="info-block">
            <div class="info-icon"><i class="pi pi-lock" /></div>
            <div>
              <div class="row-title">Прямое шифрование</div>
              <div class="row-desc">
                Обмен ключами (crypto_box) происходит при подключении к WebSocket.
                Сервер не видит открытый текст сообщений.
              </div>
            </div>
          </div>
          <div class="info-block">
            <div class="info-icon warning"><i class="pi pi-exclamation-triangle" /></div>
            <div>
              <div class="row-title">Ключи не сохраняются на диск</div>
              <div class="row-desc">
                При перезагрузке страницы создаётся новая пара ключей.
                Это безопасно, но означает, что нельзя «экспортировать» ключи.
              </div>
            </div>
          </div>
        </section>
      </div>
    </template>
  </MainLayout>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useRouter } from 'vue-router'
import Avatar from 'primevue/avatar'
import Button from 'primevue/button'
import { useToast } from 'primevue/usetoast'
import MainLayout from '@/components/layout/MainLayout.vue'
import { useAuthStore } from '@/stores/auth'
import { useWebSocketStore } from '@/stores/websocket'

type SectionId = 'profile' | 'keys'

const auth = useAuthStore()
const ws = useWebSocketStore()
const router = useRouter()
const toast = useToast()

const sections: { id: SectionId; label: string; icon: string }[] = [
  { id: 'profile', label: 'Профиль', icon: 'pi pi-user' },
  { id: 'keys', label: 'Ключи шифрования', icon: 'pi pi-lock' },
]

const active = ref<SectionId>('profile')

const displayName = computed(() => `User ${auth.userId}`)

const wsLabel = computed(() => {
  switch (ws.status) {
    case 'connected': return 'Подключено'
    case 'connecting': return 'Подключение…'
    case 'error': return 'Ошибка'
    default: return 'Отключено'
  }
})

const wsBadgeClass = computed(() => ({
  'badge-success': ws.status === 'connected',
  'badge-warning': ws.status === 'connecting',
  'badge-danger': ws.status === 'error' || ws.status === 'disconnected',
}))

async function copy(text: string) {
  try {
    await navigator.clipboard.writeText(text)
    toast.add({ severity: 'success', summary: 'Скопировано', life: 1500 })
  } catch {
    toast.add({ severity: 'error', summary: 'Не удалось скопировать', life: 2000 })
  }
}

function handleLogout() {
  auth.logout()
  router.push('/login')
}
</script>

<style scoped>
.account-sidebar {
  width: var(--sidebar-width);
  min-width: var(--sidebar-width);
  background: var(--bg-secondary);
  border-right: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  height: 100%;
}

.profile-card {
  padding: 1.5rem 1.25rem;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
  border-bottom: 1px solid var(--border-color);
}
.profile-avatar {
  background: var(--accent);
  color: white;
}
.profile-card h3 {
  margin: 0.5rem 0 0;
  font-size: 1rem;
  font-weight: 600;
  color: var(--text-main);
}

.settings-nav {
  padding: 0.75rem;
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.nav-row {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.75rem 1rem;
  background: transparent;
  border: none;
  color: var(--text-secondary);
  text-align: left;
  border-radius: var(--radius-sm);
  cursor: pointer;
  font-size: 0.95rem;
  transition: var(--transition-fast);
}
.nav-row i { font-size: 1rem; color: var(--text-muted); }
.nav-row:hover { background: var(--bg-hover); color: var(--text-main); }
.nav-row.active {
  background: rgba(46, 107, 255, 0.12);
  color: var(--accent);
  font-weight: 600;
}
.nav-row.active i { color: var(--accent); }
.nav-row.logout { color: var(--danger, #EF4444); margin-top: auto; }
.nav-row.logout i { color: var(--danger, #EF4444); }
.nav-row.logout:hover { background: rgba(239, 68, 68, 0.12); }

.account-main {
  flex: 1;
  overflow-y: auto;
  padding: 2rem;
  background: var(--bg-primary);
}
.account-section {
  max-width: 720px;
}
.account-section h2 {
  margin: 0 0 0.25rem;
  font-size: 1.4rem;
  color: var(--text-main);
}
.section-desc {
  margin: 0 0 1.5rem;
  color: var(--text-muted);
  font-size: 0.9rem;
  line-height: 1.5;
}

.field { margin-bottom: 1rem; }
.field label {
  display: block;
  font-size: 0.8rem;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 0.4rem;
}
.field-value {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.75rem 1rem;
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  color: var(--text-main);
}
.field-value code {
  font-family: var(--font-mono, monospace);
  font-size: 0.9rem;
}

.badge {
  padding: 4px 10px;
  border-radius: var(--radius-full);
  font-size: 0.75rem;
  font-weight: 600;
}
.badge-success { background: rgba(34, 197, 94, 0.12); color: #22C55E; }
.badge-warning { background: rgba(245, 158, 11, 0.12); color: #F59E0B; }
.badge-danger  { background: rgba(239, 68, 68, 0.12);  color: #EF4444; }

.info-block {
  display: flex;
  gap: 1rem;
  padding: 1rem 1.25rem;
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  margin-bottom: 0.75rem;
}
.info-icon {
  width: 40px;
  height: 40px;
  flex-shrink: 0;
  display: grid;
  place-items: center;
  border-radius: var(--radius-full);
  background: rgba(46, 107, 255, 0.12);
  color: var(--accent);
}
.info-icon.warning {
  background: rgba(245, 158, 11, 0.12);
  color: #F59E0B;
}
.row-title { font-weight: 500; color: var(--text-main); }
.row-desc  { font-size: 0.85rem; color: var(--text-muted); margin-top: 4px; line-height: 1.5; }

@media (max-width: 768px) {
  .account-sidebar { width: 100%; min-width: unset; }
}
</style>