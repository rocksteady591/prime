import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useUiStore = defineStore('ui', () => {
  const contextPanelOpen = ref(false)
  function toggleContextPanel() { contextPanelOpen.value = !contextPanelOpen.value }
  function closeContextPanel() { contextPanelOpen.value = false }
  return { contextPanelOpen, toggleContextPanel, closeContextPanel }
})