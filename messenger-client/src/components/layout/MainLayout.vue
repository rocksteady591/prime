<template>
  <div
    class="main-layout"
    @dragenter.prevent="onDragEnter"
    @dragover.prevent
    @drop.prevent="onDrop"
  >
    <VerticalNav />
    <slot name="sidebar" />
    <slot name="main" />
    <ContextPanel v-if="ui.contextPanelOpen" @close="ui.contextPanelOpen = false" />

    <Transition name="fade">
      <div v-if="dragging" class="drop-overlay">
        <div class="drop-card">
          <i class="pi pi-cloud-upload drop-icon" />
          <h3>Отпустите файл, чтобы отправить</h3>
          <p>Файл будет прикреплён к текущему чату</p>
        </div>
      </div>
    </Transition>

    <CommandPalette ref="paletteRef" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import VerticalNav from '@/components/layout/VerticalNav.vue'
import ContextPanel from '@/components/layout/ContextPanel.vue'
import CommandPalette from '@/components/layout/CommandPalette.vue'
import { useUiStore } from '@/stores/ui'

const ui = useUiStore()
const paletteRef = ref<InstanceType<typeof CommandPalette> | null>(null)

const dragging = ref(false)
let dragCounter = 0

function onDragEnter(e: DragEvent) {
  if (!e.dataTransfer?.types.includes('Files')) return
  dragCounter++
  dragging.value = true
}
function onDrop() {
  dragCounter = 0
  dragging.value = false
}

function onGlobalKey(e: KeyboardEvent) {
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') {
    e.preventDefault()
    paletteRef.value?.open()
  }
  if (e.key === 'Escape') paletteRef.value?.close()
}

function onGlobalDragEnd() {
  dragCounter = 0
  dragging.value = false
}
function onGlobalDragLeave() {
  dragCounter = Math.max(0, dragCounter - 1)
  if (dragCounter === 0) dragging.value = false
}

onMounted(() => {
  window.addEventListener('keydown', onGlobalKey)
  window.addEventListener('dragend', onGlobalDragEnd)
  window.addEventListener('dragleave', onGlobalDragLeave)
})
onBeforeUnmount(() => {
  window.removeEventListener('keydown', onGlobalKey)
  window.removeEventListener('dragend', onGlobalDragEnd)
  window.removeEventListener('dragleave', onGlobalDragLeave)
})
</script>

<style scoped>
.main-layout {
  display: flex;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background: var(--bg-primary);
  position: relative;
}

.drop-overlay {
  position: fixed; inset: 0;
  background: rgba(15, 17, 21, 0.72);
  backdrop-filter: blur(6px);
  display: grid; place-items: center;
  z-index: 9000;
}
.drop-card {
  padding: 3rem 4rem;
  border: 2px dashed var(--accent);
  border-radius: var(--radius-xl);
  background: var(--bg-secondary);
  text-align: center;
  box-shadow: var(--shadow-lg);
}
.drop-icon {
  font-size: 3rem;
  color: var(--accent);
  margin-bottom: 1rem;
  animation: bounce 1.4s ease-in-out infinite;
}
.drop-card h3 { margin: 0 0 0.5rem; color: var(--text-main); }
.drop-card p { margin: 0; color: var(--text-muted); }

.fade-enter-active, .fade-leave-active { transition: opacity var(--transition-base); }
.fade-enter-from, .fade-leave-to { opacity: 0; }

@keyframes bounce {
  0%, 100% { transform: translateY(0); }
  50%      { transform: translateY(-8px); }
}
</style>