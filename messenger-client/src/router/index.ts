import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import ChatView from '@/views/ChatView.vue'
import ContactsView from '@/views/ContactsView.vue'
import SettingsView from '@/views/SettingsView.vue'
import AccountView from '@/views/AccountView.vue'

const routes = [
  { path: '/', redirect: '/chats' },
  { path: '/login', component: () => import('@/views/LoginView.vue') },
  { path: '/register', component: () => import('@/views/RegisterView.vue') },
  { path: '/chats', name: 'chats', component: ChatView, meta: { requiresAuth: true } },
  { path: '/contacts', name: 'contacts', component: ContactsView },
  { path: '/settings', name: 'settings', component: SettingsView },
  { path: '/account', name: 'account', component: AccountView },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

router.beforeEach((to, from, next) => {
  const auth = useAuthStore()
  if (to.meta.requiresAuth && !auth.token) {
    next('/login')
  } else if ((to.path === '/login' || to.path === '/register') && auth.token) {
    next('/chats')
  } else {
    next()
  }
})

export default router