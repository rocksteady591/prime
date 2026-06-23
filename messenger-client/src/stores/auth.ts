import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useAuthStore = defineStore('auth', () => {
  const token = ref<string | null>(localStorage.getItem('auth_token'))
  const userId = ref<string | null>(localStorage.getItem('user_id'))

  function setAuth(newToken: string, newUserId: string) {
    token.value = newToken
    userId.value = newUserId
    localStorage.setItem('auth_token', newToken)
    localStorage.setItem('user_id', newUserId)
  }

  function logout() {
    token.value = null
    userId.value = null
    localStorage.removeItem('auth_token')
    localStorage.removeItem('user_id')
  }

  return { token, userId, setAuth, logout }
})