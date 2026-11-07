import { ref } from 'vue'
import type { Attachment } from '@/types/message'

const MAX_SIZE = 20 * 1024 * 1024 // 20 MB
const MAX_COUNT = 10

export function useAttachments() {
  const attachments = ref<Attachment[]>([])

  function addFiles(files: FileList | File[]) {
    const arr = Array.from(files)
    for (const file of arr) {
      if (attachments.value.length >= MAX_COUNT) break
      if (file.size > MAX_SIZE) {
        console.warn('File too large:', file.name)
        continue
      }
      const id = `att_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`
      const isImage = file.type.startsWith('image/')
      const url = URL.createObjectURL(file)
      attachments.value.push({
        id,
        name: file.name,
        size: file.size,
        type: file.type,
        url,
        preview: isImage ? url : undefined,
        progress: 100, // локально файл уже загружен
      })
    }
  }

  function remove(id: string) {
    const idx = attachments.value.findIndex(a => a.id === id)
    if (idx !== -1) {
      URL.revokeObjectURL(attachments.value[idx].url)
      attachments.value.splice(idx, 1)
    }
  }

  function clear() {
    for (const a of attachments.value) URL.revokeObjectURL(a.url)
    attachments.value = []
  }

  return { attachments, addFiles, remove, clear }
}