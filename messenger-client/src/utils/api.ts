import { useAuthStore } from '@/stores/auth'
import type { ApiError } from '@/types/api';

export interface ApiResponse<T = any> {
  data?: T;
  error?: string;
  status: number;
  ok: boolean;
}

export async function apiFetch<T = any>(
  url: string,
  options: RequestInit = {}
): Promise<ApiResponse<T>> {
  const auth = useAuthStore();

  const headers = new Headers(options.headers || {});
  if (!headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json');
  }

  const token = auth.token;
  if (token) {
    headers.set('Authorization', `Bearer ${token}`);
  }

  try {
    const response = await fetch(url, {
      ...options,
      headers,
    });

    const isJson = response.headers.get('content-type')?.includes('application/json');
    const body = isJson ? await response.json() : null;

    if (!response.ok) {
      if (response.status === 401) {
        // можно очистить токен, но пусть компонент решает
        return {
          ok: false,
          status: response.status,
          error: body?.message || body?.error || 'Unauthorized',
        };
      }
      return {
        ok: false,
        status: response.status,
        error: body?.message || body?.error || `HTTP ${response.status}`,
      };
    }

    return {
      ok: true,
      status: response.status,
      data: body as T,
    };
  } catch (err) {
    return {
      ok: false,
      status: 0,
      error: err instanceof Error ? err.message : 'Network error',
    };
  }
}