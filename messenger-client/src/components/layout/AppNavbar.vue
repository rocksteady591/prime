<template>
  <nav class="navbar">
    <router-link to="/chats" class="nav-item" active-class="active">Чаты</router-link>
    <router-link to="/contacts" class="nav-item" active-class="active">Контакты</router-link>
    <router-link to="/settings" class="nav-item" active-class="active">Настройки</router-link>
    <router-link to="/account" class="nav-item" active-class="active">Аккаунт</router-link>
    <a v-if="auth.token" class="nav-item logout" @click="handleLogout">Выйти</a>
  </nav>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const auth = useAuthStore()

function handleLogout() {
  auth.logout()
  router.push('/login').then(() => window.location.reload())
}
</script>

<style scoped>
.navbar {
  display: flex;
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border-color);
  padding: 0 1rem;
}

.nav-item {
  padding: 0.75rem 1.5rem;
  text-align: center;
  font-weight: 500;
  color: var(--text-muted);
  cursor: pointer;
  transition: var(--transition-fast);
  text-decoration: none;
  border-bottom: 2px solid transparent;
}
.nav-item:hover {
  color: var(--text-main);
  background: var(--bg-hover);
}
.nav-item.active {
  color: var(--accent);
  border-bottom-color: var(--accent);
}
.logout {
  margin-left: auto;
  color: #f87171;
}
.logout:hover {
  background: rgba(248, 113, 113, 0.1);
}
</style>