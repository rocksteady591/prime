<template>
  <main class="chat-window">
    <!-- ===== Шапка ===== -->
    <header class="chat-header">
      <div class="chat-header-content">
        <Avatar
          icon="pi pi-user"
          size="large"
          shape="circle"
          class="header-avatar"
        />
        <div class="chat-title">
          <h3>{{ chatName || 'Чат' }}</h3>
          <span v-if="isOnline" class="online-status">● В сети</span>
          <span v-else class="offline-status">Был(а) недавно</span>
        </div>
      </div>

      <div class="header-actions">
        <button
          class="icon-btn"
          :class="{ active: searchOpen }"
          title="Поиск в чате"
          @click="searchOpen = !searchOpen"
        >
          <i class="pi pi-search" />
        </button>
        <button class="icon-btn hide-mobile" title="Позвонить">
          <i class="pi pi-phone" />
        </button>
        <button
          class="icon-btn"
          :class="{ active: ui.contextPanelOpen }"
          title="Информация о чате"
          @click="ui.toggleContextPanel"
        >
          <i class="pi pi-info-circle" />
        </button>
        <button class="icon-btn" title="Ещё">
          <i class="pi pi-ellipsis-v" />
        </button>
      </div>
    </header>

    <!-- ===== Поиск по чату ===== -->
    <ChatSearch
      :open="searchOpen"
      :messages="messages"
      :container="messagesContainer"
      @close="searchOpen = false"
    />

    <!-- ===== Лента сообщений + FAB ===== -->
    <div class="messages-wrap">
      <div ref="messagesContainer" class="messages-container" @scroll="handleScroll">
        <!-- Skeleton при первой загрузке -->
        <MessageSkeleton v-if="showSkeleton" :count="8" />

        <!-- Лента -->
        <template v-else>
          <div v-if="loadingMore" class="load-more">
            <i class="pi pi-spin pi-spinner" />
          </div>

          <template
            v-for="item in timeline"
            :key="item.type === 'date' ? `d-${item.ts}` : item.msg.id"
          >
            <div v-if="item.type === 'date'" class="date-separator">
              <span>{{ item.label }}</span>
            </div>

            <MessageBubble
              v-else
              :msgId="item.msg.id!"
              :text="item.msg.text"
              :sender="item.msg.sender"
              :isOutgoing="item.msg.sender === currentUserId"
              :timestamp="item.msg.timestamp"
              :status="item.msg.status"
              :attachments="item.msg.attachments"
              :is-first-in-group="groupedInfo[item.msg.id!]?.isFirstInGroup"
              :is-last-in-group="groupedInfo[item.msg.id!]?.isLastInGroup"
              @resend="emit('resend', item.msg.id!)"
            />
          </template>

          <TypingIndicator :typing="isTyping" />

          <EmptyState
            v-if="!messages.length && !isTyping"
            icon="pi pi-comments"
            title="Пока нет сообщений"
            description="Напишите первым — история появится здесь"
          />
        </template>
      </div>

      <!-- FAB «вниз» с бейджем непрочитанных -->
      <Transition name="fab">
        <button
          v-if="showScrollDown"
          class="scroll-down-fab"
          type="button"
          aria-label="Прокрутить вниз"
          @click="scrollToBottom()"
        >
          <i class="pi pi-arrow-down" />
          <span v-if="unreadBelow" class="fab-badge">
            {{ unreadBelow > 99 ? '99+' : unreadBelow }}
          </span>
        </button>
      </Transition>
    </div>

    <!-- ===== Превью вложений ===== -->
    <AttachmentPreview
      v-if="attachments.length"
      :attachments="attachments"
      @remove="removeAttachment"
    />

    <!-- ===== Поле ввода ===== -->
    <div class="input-panel">
      <button class="icon-btn" title="Прикрепить файл" @click="openFileDialog">
        <i class="pi pi-paperclip" />
      </button>

      <div class="input-wrapper">
        <input
          v-model="newMessage"
          type="text"
          class="message-input"
          placeholder="Введите сообщение…"
          @keyup.enter="onSend"
        />

        <div class="emoji-anchor">
          <button class="icon-btn" title="Эмодзи" @click="emojiOpen = !emojiOpen">
            <i class="pi pi-face-smile" />
          </button>
          <EmojiPicker :open="emojiOpen" @pick="pickEmoji" />
        </div>
      </div>

      <button
        class="send-btn"
        :disabled="!newMessage.trim() && !attachments.length"
        @click="onSend"
      >
        <i class="pi pi-send" />
      </button>
    </div>

    <input
      ref="fileInputRef"
      type="file"
      multiple
      class="hidden-input"
      @change="onFilePick"
    />
  </main>
</template>

<script setup lang="ts">
import { ref, watch, nextTick, computed, onMounted } from 'vue'
import Avatar from 'primevue/avatar'
import MessageBubble from './MessageBubble.vue'
import TypingIndicator from './TypingIndicator.vue'
import AttachmentPreview from './AttachmentPreview.vue'
import ChatSearch from './ChatSearch.vue'
import EmojiPicker from './EmojiPicker.vue'
import MessageSkeleton from './MessageSkeleton.vue'
import EmptyState from '@/components/common/EmptyState.vue'
import { useUiStore } from '@/stores/ui'
import { useAttachments } from '@/composables/useAttachments'
import type { Message, Attachment } from '@/types/message'

// ============================================================
//  Props & Emits — ВСЕГДА первыми
// ============================================================
const props = defineProps<{
  messages: readonly Message[]
  currentUserId: string
  chatName?: string
  isTyping?: boolean
  isOnline?: boolean
  loadingMore?: boolean
  isLoadingHistory?: boolean
}>()

const emit = defineEmits<{
  (e: 'send', text: string, attachments: Attachment[]): void
  (e: 'loadMore'): void
  (e: 'resend', msgId: string): void
}>()

// ============================================================
//  Состояние
// ============================================================
const ui = useUiStore()
const newMessage = ref('')
const messagesContainer = ref<HTMLElement | null>(null)
const fileInputRef = ref<HTMLInputElement | null>(null)
const searchOpen = ref(false)
const emojiOpen = ref(false)

const showScrollDown = ref(false)
const unreadBelow = ref(0)
const AT_BOTTOM_THRESHOLD = 120

const {
  attachments,
  addFiles,
  remove: removeAttachment,
  clear: clearAttachments,
} = useAttachments()

let prevHeight = 0
let loadingMoreLocal = false

// ============================================================
//  Computed
// ============================================================
const showSkeleton = computed(
  () => props.isLoadingHistory && props.messages.length === 0,
)

type MessageGroupInfo = { isFirstInGroup: boolean; isLastInGroup: boolean }

const groupedInfo = computed<Record<string, MessageGroupInfo>>(() => {
  const map: Record<string, MessageGroupInfo> = {}
  const list = props.messages

  for (let i = 0; i < list.length; i++) {
    const cur = list[i]
    const prev = list[i - 1]
    const next = list[i + 1]
    const key = cur.id!
    if (!key) continue

    const breakWithPrev = !prev
      || prev.sender !== cur.sender
      || cur.timestamp - prev.timestamp > 5 * 60_000

    const breakWithNext = !next
      || next.sender !== cur.sender
      || next.timestamp - cur.timestamp > 5 * 60_000

    map[key] = {
      isFirstInGroup: breakWithPrev,
      isLastInGroup: breakWithNext,
    }
  }
  return map
})

type DateItem =
  | { type: 'date'; label: string; ts: number }
  | { type: 'msg'; msg: Message }

const timeline = computed<DateItem[]>(() => {
  const result: DateItem[] = []
  let lastDate: string | null = null
  for (const m of props.messages) {
    const d = new Date(m.timestamp)
    const dayKey = d.toDateString()
    if (dayKey !== lastDate) {
      result.push({ type: 'date', label: formatDateLabel(d), ts: m.timestamp })
      lastDate = dayKey
    }
    result.push({ type: 'msg', msg: m })
  }
  return result
})

function formatDateLabel(d: Date): string {
  const now = new Date()
  const today = new Date(now.getFullYear(), now.getMonth(), now.getDate())
  const target = new Date(d.getFullYear(), d.getMonth(), d.getDate())
  const diffDays = Math.round((today.getTime() - target.getTime()) / 86_400_000)
  if (diffDays === 0) return 'Сегодня'
  if (diffDays === 1) return 'Вчера'
  if (diffDays < 7) return d.toLocaleDateString('ru', { weekday: 'long' })
  return d.toLocaleDateString('ru', { day: 'numeric', month: 'long' })
}

// ============================================================
//  Скролл-логика
// ============================================================
function isAtBottom(): boolean {
  const c = messagesContainer.value
  if (!c) return true
  return c.scrollHeight - c.scrollTop - c.clientHeight < AT_BOTTOM_THRESHOLD
}

function scrollToBottom(smooth = true) {
  const c = messagesContainer.value
  if (!c) return
  const prefersReducedMotion = window.matchMedia(
    '(prefers-reduced-motion: reduce)',
  ).matches
  c.scrollTo({
    top: c.scrollHeight,
    behavior: smooth && !prefersReducedMotion ? 'smooth' : 'auto',
  })
  unreadBelow.value = 0
  showScrollDown.value = false
}

function handleScroll() {
  const c = messagesContainer.value
  if (!c) return

  // 1) FAB видимость
  const atBottom = isAtBottom()
  showScrollDown.value = !atBottom
  if (atBottom) unreadBelow.value = 0

  // 2) Пагинация — только когда доскроллили до самого верха
  if (c.scrollTop <= 50 && !loadingMoreLocal) {
    loadingMoreLocal = true
    prevHeight = c.scrollHeight
    emit('loadMore')
  }
}

// ============================================================
//  Действия
// ============================================================
function onSend() {
  const text = newMessage.value.trim()
  if (!text && !attachments.value.length) return
  emit('send', text, [...attachments.value])
  newMessage.value = ''
  clearAttachments()
}

function openFileDialog() {
  fileInputRef.value?.click()
}

function onFilePick(e: Event) {
  const input = e.target as HTMLInputElement
  if (input.files) addFiles(input.files)
  input.value = ''
}

function pickEmoji(emoji: string) {
  newMessage.value += emoji
  emojiOpen.value = false
}

// ============================================================
//  Реакция на новые сообщения
// ============================================================
watch(
  () => props.messages.length,
  (newLen, oldLen) => {
    if (!messagesContainer.value) return
    const c = messagesContainer.value
    const added = newLen - oldLen

    // 1) Первая загрузка чата — мгновенный скролл вниз, без счётчика
    if (oldLen === 0) {
      nextTick(() => scrollToBottom(false))
      return
    }

    // 2) Пагинация — сохраняем видимую позицию
    if (loadingMoreLocal) {
      nextTick(() => {
        c.scrollTop = c.scrollHeight - prevHeight
        loadingMoreLocal = false
      })
      return
    }

    if (added <= 0) return

    // 3) Своё сообщение — всегда скроллим вниз
    const last = props.messages[newLen - 1]
    if (last?.sender === props.currentUserId) {
      nextTick(() => scrollToBottom(true))
      return
    }

    // 4) Чужое сообщение
    if (isAtBottom()) {
      nextTick(() => scrollToBottom(true))
    } else {
      // пользователь читает историю — не дёргаем, только бейдж
      unreadBelow.value += added
      showScrollDown.value = true
    }
  },
)

// ============================================================
//  Первичный скролл при маунте
// ============================================================
onMounted(() => {
  if (props.messages.length) {
    nextTick(() => scrollToBottom(false))
  }
})
</script>

<style scoped>
/* ============================================================ */
/*  Каркас */
/* ============================================================ */
.chat-window {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  background: var(--bg-primary);
}

/* ============================================================ */
/*  Шапка */
/* ============================================================ */
.chat-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  height: var(--header-height);
  padding: 0 1.25rem;
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border-color);
  backdrop-filter: blur(8px);
  flex-shrink: 0;
}

.chat-header-content {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  min-width: 0;
}

.header-avatar {
  background: var(--accent);
  color: #fff;
  flex-shrink: 0;
}

.chat-title {
  min-width: 0;
}
.chat-title h3 {
  margin: 0;
  font-size: 1rem;
  font-weight: 600;
  color: var(--text-main);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.online-status {
  font-size: 0.72rem;
  color: var(--success);
}
.offline-status {
  font-size: 0.72rem;
  color: var(--text-muted);
}

.header-actions {
  display: flex;
  gap: 2px;
  flex-shrink: 0;
}

.icon-btn {
  width: 36px;
  height: 36px;
  display: grid;
  place-items: center;
  background: transparent;
  border: none;
  cursor: pointer;
  border-radius: var(--radius-full);
  color: var(--text-muted);
  transition: var(--transition-fast);
}
.icon-btn:hover {
  background: var(--bg-hover);
  color: var(--text-main);
}
.icon-btn.active {
  background: var(--accent-soft);
  color: var(--accent);
}

/* ============================================================ */
/*  Лента сообщений + FAB */
/* ============================================================ */
.messages-wrap {
  flex: 1;
  min-height: 0;
  position: relative;
  display: flex;
  flex-direction: column;
}

.messages-container {
  flex: 1;
  overflow-y: auto;
  padding: 1.5rem 2rem;
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
  min-height: 0;
  /* scroll-behavior: smooth НЕ ставим — ломает восстановление позиции при пагинации */
}

.load-more {
  display: flex;
  justify-content: center;
  padding: 0.5rem;
  color: var(--text-muted);
  font-size: 1.1rem;
}

.date-separator {
  display: flex;
  justify-content: center;
  margin: 1rem 0 0.5rem;
  position: sticky;
  top: 0.5rem;
  z-index: 1;
}
.date-separator span {
  padding: 4px 12px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-full);
  font-size: 0.72rem;
  color: var(--text-muted);
  font-weight: 500;
  backdrop-filter: blur(8px);
}

/* FAB «вниз» */
.scroll-down-fab {
  position: absolute;
  right: 1.25rem;
  bottom: 1.25rem;
  width: 44px;
  height: 44px;
  border-radius: 50%;
  background: var(--bg-elevated);
  border: 1px solid var(--border-strong);
  color: var(--accent);
  cursor: pointer;
  box-shadow: var(--shadow-lg);
  display: grid;
  place-items: center;
  transition: var(--transition-fast);
  z-index: 5;
}
.scroll-down-fab:hover {
  transform: translateY(-2px);
  background: var(--accent-soft);
}
.scroll-down-fab:active {
  transform: scale(0.94);
}

.fab-badge {
  position: absolute;
  top: -4px;
  right: -4px;
  min-width: 20px;
  height: 20px;
  padding: 0 5px;
  background: var(--accent);
  color: #fff;
  border-radius: var(--radius-full);
  font-size: 0.68rem;
  font-weight: 700;
  display: grid;
  place-items: center;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
}

.fab-enter-active,
.fab-leave-active {
  transition: all var(--transition-base) var(--ease-spring);
}
.fab-enter-from,
.fab-leave-to {
  opacity: 0;
  transform: translateY(12px) scale(0.8);
}

/* ============================================================ */
/*  Поле ввода */
/* ============================================================ */
.input-panel {
  display: flex;
  align-items: flex-end;
  gap: 0.5rem;
  padding: 0.75rem 1.25rem;
  background: var(--bg-secondary);
  border-top: 1px solid var(--border-color);
  flex-shrink: 0;
}

.input-wrapper {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.4rem 0.4rem 0.4rem 1rem;
  background: var(--bg-primary);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-xl);
  transition: var(--transition-fast);
}
.input-wrapper:focus-within {
  border-color: var(--accent);
  box-shadow: var(--shadow-glow);
}

.message-input {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  color: var(--text-main);
  font-family: inherit;
  font-size: 0.95rem;
  padding: 0.45rem 0;
}
.message-input::placeholder {
  color: var(--text-faint);
}

.send-btn {
  width: 42px;
  height: 42px;
  flex-shrink: 0;
  display: grid;
  place-items: center;
  background: linear-gradient(135deg, var(--accent), var(--accent-hover));
  color: #fff;
  border: none;
  border-radius: var(--radius-full);
  cursor: pointer;
  transition: var(--transition-fast);
  box-shadow: 0 4px 14px -4px var(--accent-glow);
}
.send-btn:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 6px 20px -4px var(--accent-glow);
}
.send-btn:active:not(:disabled) {
  transform: scale(0.96);
}
.send-btn:disabled {
  opacity: 0.35;
  cursor: not-allowed;
  box-shadow: none;
}

.hidden-input {
  display: none;
}
.emoji-anchor {
  position: relative;
}

/* ============================================================ */
/*  Адаптив */
/* ============================================================ */
@media (max-width: 768px) {
  .chat-header {
    padding: 0 0.75rem;
  }
  .hide-mobile {
    display: none;
  }
  .messages-container {
    padding: 1rem;
  }
  .input-panel {
    padding: 0.5rem 0.75rem;
  }
  .scroll-down-fab {
    right: 0.75rem;
    bottom: 0.75rem;
    width: 40px;
    height: 40px;
  }
}
</style>