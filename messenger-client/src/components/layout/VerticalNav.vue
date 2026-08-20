<template>
  <nav class="vertical-nav">
    <router-link to="/chats" class="nav-item" active-class="active">
      <i class="pi pi-comment" />
      <span>Чаты</span>
    </router-link>
    <router-link to="/contacts" class="nav-item" active-class="active">
      <i class="pi pi-users" />
      <span>Контакты</span>
    </router-link>
    <router-link to="/settings" class="nav-item" active-class="active">
      <i class="pi pi-cog" />
      <span>Настройки</span>
    </router-link>
    <router-link to="/account" class="nav-item" active-class="active">
      <i class="pi pi-user" />
      <span>Аккаунт</span>
    </router-link>
    <a v-if="auth.token" class="nav-item logout" @click="handleLogout">
      <i class="pi pi-sign-out" />
      <span>Выйти</span>
    </a>
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
.vertical-nav {
  width: 72px;
  min-width: 72px;
  height: 100%;
  background: var(--bg-secondary);
  border-right: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 1rem 0;
  gap: 0.5rem;
}

.nav-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 56px;
  height: 56px;
  border-radius: var(--radius-sm);
  color: var(--text-muted);
  text-decoration: none;
  transition: var(--transition-fast);
  font-size: 0.65rem;
  gap: 2px;
}
.nav-item i {
  font-size: 1.3rem;
}
.nav-item:hover {
  background: var(--bg-hover);
  color: var(--text-main);
}
.nav-item.active {
  color: var(--accent);
  background: var(--accent-glow);
}
.logout {
  margin-top: auto;
  color: #f87171;
}
.logout:hover {
  background: rgba(248, 113, 113, 0.1);
}
</style>