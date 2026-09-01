import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'

export async function apiFetch(url: string, options: RequestInit = {}) {
    const auth = useAuthStore()
    const router = useRouter()

    const headers = new Headers(options.headers || {})

    if (!headers.has('Content-Type')) {
        headers.set('Content-Type', 'application/json')
    }

    const token = auth.token
    if (token) {
        headers.set('Authorization', `Bearer ${token}`)
    }

    const response = await fetch(url, {
        ...options,
        headers,
    })

    if (response.status === 401) {
        auth.logout()
        router.push('/login')
        throw new Error('Unauthorized')
    }

    return response
}