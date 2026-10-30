import { apiFetch } from './api'

export async function uploadFile(file: File): Promise<string> {
  const form = new FormData()
  form.append('file', file)
  const res = await apiFetch<{ url: string }>('/api/upload', {
    method: 'POST',
    body: form,
    headers: {},
  })
  if (!res.ok || !res.data?.url) throw new Error(res.error || 'Upload failed')
  return res.data.url
}