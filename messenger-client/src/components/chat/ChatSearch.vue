<template>
  <Transition name="slide-down">
    <div v-if="open" class="chat-search">
      <i class="pi pi-search" />
      <input
        ref="inputRef"
        v-model="query"
        placeholder="Поиск по сообщениям…"
        @keydown.enter="next"
        @keydown.escape="$emit('close')"
      />
      <span v-if="query" class="count">
        {{ total ? `${currentIndex + 1} из ${total}` : '0 результатов' }}
      </span>
      <button class="icon-btn" :disabled="!total" @click="prev">
        <i class="pi pi-chevron-up" />
      </button>
      <button class="icon-btn" :disabled="!total" @click="next">
        <i class="pi pi-chevron-down" />
      </button>
      <button class="icon-btn" @click="$emit('close')">
        <i class="pi pi-times" />
      </button>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick, onMounted } from 'vue'
import type { Message } from '@/types/message'

const props = defineProps<{
  open: boolean
  messages: readonly Message[]
  container: HTMLElement | null
}>()
defineEmits<{ (e: 'close'): void }>()

const query = ref('')
const currentIndex = ref(0)
const inputRef = ref<HTMLInputElement | null>(null)

const matches = computed(() => {
  const q = query.value.trim().toLowerCase()
  if (!q) return [] as Message[]
  return props.messages.filter(m => m.text.toLowerCase().includes(q))
})
const total = computed(() => matches.value.length)

function focusMessage(msg: Message) {
  if (!props.container) return
  const el = props.container.querySelector(`[data-msg-id="${msg.id}"]`)
  el?.scrollIntoView({ behavior: 'smooth', block: 'center' })
  el?.classList.add('search-flash')
  setTimeout(() => el?.classList.remove('search-flash'), 1200)
}

function next() {
  if (!total.value) return
  currentIndex.value = (currentIndex.value + 1) % total.value
  focusMessage(matches.value[currentIndex.value])
}
function prev() {
  if (!total.value) return
  currentIndex.value = (currentIndex.value - 1 + total.value) % total.value
  focusMessage(matches.value[currentIndex.value])
}

watch(() => props.open, async v => {
  if (v) {
    await nextTick()
    inputRef.value?.focus()
  } else {
    query.value = ''
    currentIndex.value = 0
  }
})
watch(query, () => {
  currentIndex.value = 0
  if (matches.value.length) focusMessage(matches.value[0])
})
</script>

<style scoped>
.chat-search {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1.25rem;
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border-color);
  color: var(--text-muted);
}
.chat-search input {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  color: var(--text-main);
  font-family: inherit;
  font-size: 0.9rem;
}
.count { font-size: 0.75rem; }
.icon-btn {
  width: 30px; height: 30px;
  display: grid; place-items: center;
  background: transparent; border: none; cursor: pointer;
  border-radius: var(--radius-full);
  color: var(--text-muted);
  transition: var(--transition-fast);
}
.icon-btn:hover:not(:disabled) { background: var(--bg-hover); color: var(--text-main); }
.icon-btn:disabled { opacity: 0.3; cursor: not-allowed; }

.slide-down-enter-active, .slide-down-leave-active {
  transition: all var(--transition-base) var(--ease-out);
}
.slide-down-enter-from, .slide-down-leave-to {
  transform: translateY(-100%);
  opacity: 0;
}
</style>