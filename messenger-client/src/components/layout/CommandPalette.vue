<template>
  <Transition name="palette">
    <div v-if="visible" class="palette-backdrop" @click.self="close">
      <div class="palette" role="dialog" aria-label="Быстрый поиск">
        <div class="palette-search">
          <i class="pi pi-search" />
          <input
            ref="inputRef"
            v-model="query"
            placeholder="Поиск по чатам, контактам и командам…"
            @keydown.down.prevent="move(1)"
            @keydown.up.prevent="move(-1)"
            @keydown.enter="run"
          />
          <kbd>Esc</kbd>
        </div>
        <div class="palette-list" v-if="filtered.length">
          <button
            v-for="(item, i) in filtered"
            :key="item.id"
            class="palette-item"
            :class="{ active: i === cursor }"
            @mouseenter="cursor = i"
            @click="select(item)"
          >
            <i :class="item.icon" />
            <span class="label">{{ item.label }}</span>
            <span class="hint">{{ item.hint }}</span>
          </button>
        </div>
        <div v-else class="palette-empty">
          <i class="pi pi-inbox" />
          <span>Ничего не найдено</span>
        </div>
      </div>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { ref, computed, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import { useChatsStore } from '@/stores/chat'

const visible = ref(false)
const query = ref('')
const cursor = ref(0)
const inputRef = ref<HTMLInputElement | null>(null)
const router = useRouter()
const chats = useChatsStore()

type Item = { id: string; label: string; icon: string; hint?: string; action: () => void }

const actions = computed<Item[]>(() => [
  { id: 'go-contacts', label: 'Открыть контакты', icon: 'pi pi-users', hint: 'Перейти', action: () => router.push('/contacts') },
  { id: 'go-settings', label: 'Настройки', icon: 'pi pi-cog', hint: 'Перейти', action: () => router.push('/settings') },
  { id: 'go-account', label: 'Аккаунт', icon: 'pi pi-user', hint: 'Перейти', action: () => router.push('/account') },
])

const filtered = computed<Item[]>(() => {
  const q = query.value.trim().toLowerCase()
  const chatsItems: Item[] = chats.chatList.map(c => ({
    id: c.id,
    label: c.name,
    icon: 'pi pi-comment',
    hint: 'Чат',
    action: () => { chats.setActiveChat(c.id); router.push('/chats') },
  }))
  const all = [...chatsItems, ...actions.value]
  return q ? all.filter(i => i.label.toLowerCase().includes(q)) : all
})

function open() {
  visible.value = true
  query.value = ''
  cursor.value = 0
  nextTick(() => inputRef.value?.focus())
}
function close() { visible.value = false }
function move(delta: number) {
  if (!filtered.value.length) return
  cursor.value = (cursor.value + delta + filtered.value.length) % filtered.value.length
}
function run() { select(filtered.value[cursor.value]) }
function select(item?: Item) {
  if (!item) return
  item.action()
  close()
}

defineExpose({ open, close })
</script>

<style scoped>
.palette-backdrop {
  position: fixed; inset: 0;
  background: rgba(15, 17, 21, 0.6);
  backdrop-filter: blur(4px);
  display: grid; place-items: start center;
  padding-top: 12vh;
  z-index: 9500;
}
.palette {
  width: min(640px, 92vw);
  background: var(--bg-elevated);
  border: 1px solid var(--border-strong);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-lg);
  overflow: hidden;
}
.palette-search {
  display: flex; align-items: center; gap: 0.75rem;
  padding: 1rem 1.25rem;
  border-bottom: 1px solid var(--border-color);
  color: var(--text-muted);
}
.palette-search input {
  flex: 1; background: transparent; border: none; outline: none;
  color: var(--text-main); font-size: 1rem; font-family: inherit;
}
.palette-search kbd {
  font-family: var(--font-mono); font-size: 0.7rem;
  background: var(--bg-secondary); border: 1px solid var(--border-color);
  border-radius: var(--radius-xs); padding: 2px 6px; color: var(--text-muted);
}
.palette-list { max-height: 50vh; overflow-y: auto; padding: 0.4rem; }
.palette-item {
  width: 100%;
  display: flex; align-items: center; gap: 0.75rem;
  padding: 0.7rem 0.9rem;
  background: transparent; border: none; cursor: pointer;
  color: var(--text-main); text-align: left;
  border-radius: var(--radius-sm);
  font-family: inherit; font-size: 0.9rem;
  transition: var(--transition-fast);
}
.palette-item:hover,
.palette-item.active { background: var(--accent-soft); color: var(--accent); }
.palette-item i { font-size: 1rem; }
.palette-item .label { flex: 1; }
.palette-item .hint { font-size: 0.75rem; color: var(--text-muted); }
.palette-empty { padding: 2rem; text-align: center; color: var(--text-muted); display: flex; flex-direction: column; gap: 0.5rem; align-items: center; }

.palette-enter-active, .palette-leave-active { transition: opacity var(--transition-base); }
.palette-enter-active .palette, .palette-leave-active .palette { transition: transform var(--transition-base) var(--ease-out); }
.palette-enter-from, .palette-leave-to { opacity: 0; }
.palette-enter-from .palette, .palette-leave-to .palette { transform: translateY(-12px) scale(0.98); }
</style>