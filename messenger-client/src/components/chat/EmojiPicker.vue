<template>
  <Transition name="pop">
    <div v-if="open" class="emoji-pop" ref="popRef">
      <div class="emoji-tabs">
        <button
          v-for="cat in categories"
          :key="cat.id"
          class="emoji-tab"
          :class="{ active: activeCat === cat.id }"
          @click="activeCat = cat.id"
        >
          {{ cat.icon }}
        </button>
      </div>
      <div class="emoji-grid">
        <button
          v-for="e in currentEmojis"
          :key="e"
          class="emoji-btn"
          @click="$emit('pick', e)"
        >
          {{ e }}
        </button>
      </div>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'

const props = defineProps<{ open: boolean }>()
defineEmits<{ (e: 'pick', emoji: string): void }>()

const popRef = ref<HTMLElement | null>(null)
const activeCat = ref('smile')

const categories = [
  { id: 'smile', icon: '😀', emojis: ['😀','😃','😄','😁','😆','😅','🤣','😂','🙂','🙃','😉','😊','😇','🥰','😍','🤩','😘','😗','😚','😙','🥲','😋','😛','😜','🤪','😝','🤑','🤗','🤭','🤫','🤔','🤐','🤨','😐','😑','😶','😏','😒','🙄','😬','🤥'] },
  { id: 'gesture', icon: '👍', emojis: ['👍','👎','👌','✌️','🤞','🤟','🤘','🤙','👈','👉','👆','👇','☝️','✋','🤚','🖐️','🖖','👋','🤝','🙏','💪','🦾','🖕','✍️','🤳'] },
  { id: 'heart', icon: '❤️', emojis: ['❤️','🧡','💛','💚','💙','💜','🖤','🤍','🤎','💔','❣️','💕','💞','💓','💗','💖','💘','💝','💟','♥️','💌'] },
  { id: 'object', icon: '🔥', emojis: ['🔥','✨','⭐','🌟','💫','⚡','💥','💯','🎉','🎊','🎁','🏆','🥇','💎','🔔','📌','📎','🔗','💡','📷'] },
] as const

const currentEmojis = computed(() =>
  categories.find(c => c.id === activeCat.value)?.emojis ?? [],
)

function onDocClick(e: MouseEvent) {
  if (!props.open) return
  if (popRef.value && !popRef.value.contains(e.target as Node)) {
    // родитель сам решает — emit close через отдельное событие, но проще пусть родитель слушает
  }
}
onMounted(() => document.addEventListener('mousedown', onDocClick))
onBeforeUnmount(() => document.removeEventListener('mousedown', onDocClick))
</script>

<style scoped>
.emoji-pop {
  position: absolute;
  bottom: calc(100% + 8px);
  right: 0;
  width: 320px;
  max-height: 320px;
  background: var(--bg-elevated);
  border: 1px solid var(--border-strong);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-lg);
  display: flex;
  flex-direction: column;
  overflow: hidden;
  z-index: 100;
}
.emoji-tabs {
  display: flex;
  border-bottom: 1px solid var(--border-color);
  background: var(--bg-secondary);
}
.emoji-tab {
  flex: 1;
  padding: 0.6rem;
  background: transparent;
  border: none;
  cursor: pointer;
  font-size: 1.1rem;
  transition: var(--transition-fast);
}
.emoji-tab:hover { background: var(--bg-hover); }
.emoji-tab.active { background: var(--accent-soft); }
.emoji-grid {
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 2px;
  padding: 0.5rem;
  overflow-y: auto;
}
.emoji-btn {
  aspect-ratio: 1;
  display: grid;
  place-items: center;
  background: transparent;
  border: none;
  cursor: pointer;
  font-size: 1.1rem;
  border-radius: var(--radius-sm);
  transition: var(--transition-fast);
}
.emoji-btn:hover {
  background: var(--bg-hover);
  transform: scale(1.15);
}
.pop-enter-active, .pop-leave-active {
  transition: all var(--transition-base) var(--ease-spring);
}
.pop-enter-from, .pop-leave-to {
  opacity: 0;
  transform: translateY(6px) scale(0.96);
}
</style>