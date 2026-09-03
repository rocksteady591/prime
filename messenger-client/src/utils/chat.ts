/**
 * Генерирует стабильный ID чата на основе двух ID пользователей.
 * Сортировка гарантирует, что для пары (A,B) всегда будет один ID.
 */
export function getChatId(userIdA: string | number, userIdB: string | number): string {
  const ids = [Number(userIdA), Number(userIdB)].sort((a, b) => a - b);
  return `chat_${ids[0]}_${ids[1]}`;
}

/**
 * Извлекает ID собеседника из chatId и текущего userId.
 */
export function getRecipientId(chatId: string, myId: string): string | null {
  const parts = chatId.split('_');
  if (parts.length !== 3) return null;
  const id1 = parts[1];
  const id2 = parts[2];
  return id1 === myId ? id2 : id1;
}