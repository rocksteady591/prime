<template>
  <MainLayout>
    <template #sidebar>
      <aside class="settings-sidebar">
        <div class="sidebar-header">
          <h2>Настройки</h2>
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
        </div>
      </aside>
    </template>

    <template #main>
      <div class="settings-main">
        <!-- Тема -->
        <section v-if="active === 'theme'" class="settings-section">
          <h2>Тема оформления</h2>
          <p class="section-desc">Выберите, как будет выглядеть интерфейс</p>
          <div class="theme-options">
            <button
              class="theme-card"
              :class="{ active: theme === 'dark' }"
              @click="setTheme('dark')"
            >
              <div class="theme-preview dark" />
              <span>Тёмная</span>
            </button>
            <button
              class="theme-card"
              :class="{ active: theme === 'light' }"
              @click="setTheme('light')"
            >
              <div class="theme-preview light" />
              <span>Светлая</span>
            </button>
          </div>
        </section>

        <!-- Уведомления -->
        <section v-if="active === 'notifications'" class="settings-section">
          <h2>Уведомления</h2>
          <p class="section-desc">Управление оповещениями</p>
          <div class="setting-row">
            <div>
              <div class="row-title">Уведомления о новых сообщениях</div>
              <div class="row-desc">Показывать бейдж непрочитанных</div>
            </div>
            <ToggleSwitch v-model="notifyMessages" @update:modelValue="persist" />
          </div>
          <div class="setting-row">
            <div>
              <div class="row-title">Звук</div>
              <div class="row-desc">Воспроизводить звук при новом сообщении</div>
            </div>
            <ToggleSwitch v-model="soundOnMessage" @update:modelValue="persist" />
          </div>
          <div class="setting-row">
            <div>
              <div class="row-title">Показывать «печатает…»</div>
              <div class="row-desc">Индикатор набора текста собеседником</div>
            </div>
            <ToggleSwitch v-model="showTyping" @update:modelValue="persist" />
          </div>
        </section>

        <!-- Безопасность -->
        <section v-if="active === 'security'" class="settings-section">
          <h2>Безопасность</h2>
          <p class="section-desc">Шифрование и приватность</p>
          <div class="setting-row">
            <div>
              <div class="row-title">Сквозное шифрование</div>
              <div class="row-desc">Все сообщения шифруются X25519 + XSalsa20-Poly1305</div>
            </div>
            <span class="badge badge-success">Включено</span>
          </div>
          <div class="setting-row">
            <div>
              <div class="row-title">Отправка по Enter</div>
              <div class="row-desc">Enter — отправить, Shift+Enter — новая строка</div>
            </div>
            <ToggleSwitch v-model="enterToSend" @update:modelValue="persist" />
          </div>
        </section>
      </div>
    </template>
  </MainLayout>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import ToggleSwitch from 'primevue/toggleswitch'
import MainLayout from '@/components/layout/MainLayout.vue'

type SectionId = 'theme' | 'notifications' | 'security'

const sections: { id: SectionId; label: string; icon: string }[] = [
  { id: 'theme', label: 'Тема', icon: 'pi pi-palette' },
  { id: 'notifications', label: 'Уведомления', icon: 'pi pi-bell' },
  { id: 'security', label: 'Безопасность', icon: 'pi pi-shield' },
]

const active = ref<SectionId>('theme')

const theme = ref<'dark' | 'light'>('dark')
const notifyMessages = ref(true)
const soundOnMessage = ref(true)
const showTyping = ref(true)
const enterToSend = ref(true)

function loadPrefs() {
  const t = localStorage.getItem('theme')
  theme.value = t === 'light' ? 'light' : 'dark'
  notifyMessages.value = localStorage.getItem('notify_messages') !== '0'
  soundOnMessage.value = localStorage.getItem('sound_on_message') !== '0'
  showTyping.value = localStorage.getItem('show_typing') !== '0'
  enterToSend.value = localStorage.getItem('enter_to_send') !== '0'
}

function persist() {
  localStorage.setItem('notify_messages', notifyMessages.value ? '1' : '0')
  localStorage.setItem('sound_on_message', soundOnMessage.value ? '1' : '0')
  localStorage.setItem('show_typing', showTyping.value ? '1' : '0')
  localStorage.setItem('enter_to_send', enterToSend.value ? '1' : '0')
}

function setTheme(t: 'dark' | 'light') {
  theme.value = t
  document.documentElement.classList.toggle('dark-mode', t === 'dark')
  localStorage.setItem('theme', t)
}

onMounted(loadPrefs)
</script>

<style scoped>
.settings-sidebar {
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
  background: var(--accent-soft, rgba(46,107,255,0.12));
  color: var(--accent);
  font-weight: 600;
}
.nav-row.active i { color: var(--accent); }

.settings-main {
  flex: 1;
  overflow-y: auto;
  padding: 2rem;
  background: var(--bg-primary);
}

.settings-section {
  max-width: 720px;
}
.settings-section h2 {
  margin: 0 0 0.25rem;
  font-size: 1.4rem;
  color: var(--text-main);
}
.section-desc {
  margin: 0 0 1.5rem;
  color: var(--text-muted);
  font-size: 0.9rem;
}

.theme-options {
  display: flex;
  gap: 1rem;
}
.theme-card {
  flex: 1;
  padding: 0.75rem;
  background: var(--bg-secondary);
  border: 2px solid var(--border-color);
  border-radius: var(--radius-md);
  cursor: pointer;
  color: var(--text-main);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
  font-weight: 500;
  transition: var(--transition-fast);
}
.theme-card:hover { border-color: var(--accent); }
.theme-card.active { border-color: var(--accent); box-shadow: 0 0 0 3px var(--accent-glow); }
.theme-preview {
  width: 100%;
  height: 90px;
  border-radius: var(--radius-sm);
  border: 1px solid var(--border-color);
}
.theme-preview.dark  { background: linear-gradient(135deg, #0F1115 0%, #22262F 100%); }
.theme-preview.light { background: linear-gradient(135deg, #FFFFFF 0%, #E5E4E7 100%); }

.setting-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 1rem;
  padding: 1rem 1.25rem;
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  margin-bottom: 0.75rem;
}
.row-title { font-weight: 500; color: var(--text-main); }
.row-desc  { font-size: 0.85rem; color: var(--text-muted); margin-top: 2px; }

.badge {
  padding: 4px 10px;
  border-radius: var(--radius-full);
  font-size: 0.75rem;
  font-weight: 600;
}
.badge-success {
  background: rgba(34, 197, 94, 0.12);
  color: #22C55E;
}
</style>