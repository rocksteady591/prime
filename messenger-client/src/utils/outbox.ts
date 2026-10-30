import type { Message } from '@/types/message'

const KEY = 'messenger_outbox'

export function getOutbox(): Record<string, Message[]> {
  try { return JSON.parse(localStorage.getItem(KEY) || '{}') } catch { return {} }
}

function save(ob: Record<string, Message[]>) {
  localStorage.setItem(KEY, JSON.stringify(ob))
}

export function addToOutbox(chatId: string, msg: Message) {
  const ob = getOutbox()
  ob[chatId] = [...(ob[chatId] || []), msg]
  save(ob)
}

export function removeFromOutbox(chatId: string, msgId: string) {
  const ob = getOutbox()
  if (!ob[chatId]) return
  ob[chatId] = ob[chatId].filter(m => m.id !== msgId)
  if (!ob[chatId].length) delete ob[chatId]
  save(ob)
}

export function updateOutboxStatus(chatId: string, msgId: string, status: Message['status']) {
  const ob = getOutbox()
  const list = ob[chatId]
  if (!list) return
  const idx = list.findIndex(m => m.id === msgId)
  if (idx !== -1) {
    list[idx] = { ...list[idx], status }
    save(ob)
  }
}