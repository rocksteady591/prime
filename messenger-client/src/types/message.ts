export interface Attachment {
  id: string
  name: string
  size: number
  type: string      // MIME
  url: string       // objectURL для превью (blob) или base64
  preview?: string  // для изображений — тот же objectURL
  progress?: number // 0..100 при загрузке
}

export interface Message {
  id?: string
  sender: string
  text: string
  timestamp: number
  recipient?: string
  status?: 'pending' | 'sent' | 'delivered' | 'error'
  attachments?: readonly Attachment[]
}