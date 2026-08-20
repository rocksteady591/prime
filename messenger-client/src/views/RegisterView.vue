<template>
    <div class="auth-wrapper">
        <Card class="auth-card">
            <template #title>
                Регистрация
            </template>
            <template #content>
                <div class="p-fluid">
                    <div class="field">
                        <label for="login">Логин</label>
                        <InputText id="login" v-model="login" />
                    </div>
                    <div class="field">
                        <label for="password">Пароль</label>
                        <Password id="password" v-model="password" toggleMask :feedback="false" />
                    </div>
                    <div class="field">
                        <label for="passwordConfirm">Подтверждение пароля</label>
                        <Password id="passwordConfirm" v-model="passwordConfirm" :feedback="false" toggleMask />
                    </div>
                </div>
            </template>
            <template #footer>
                <Button label="Зарегистрироваться"
                        icon="pi pi-user-plus"
                        @click="handleRegister"
                        :loading="loading"
                        :disabled="loading" />
                <p class="login-link">
                    Уже есть аккаунт? <router-link to="/login">Войти</router-link>
                </p>
            </template>
        </Card>
        <Toast />
    </div>
</template>

<script setup lang="ts">
    import { ref } from 'vue'
    import { useRouter } from 'vue-router'
    import { useAuthStore } from '@/stores/auth'
    import { useToast } from 'primevue/usetoast'
    import Card from 'primevue/card'
    import InputText from 'primevue/inputtext'
    import Password from 'primevue/password'
    import Button from 'primevue/button'
    import Toast from 'primevue/toast'

    const login = ref('')
    const password = ref('')
    const passwordConfirm = ref('')
    const loading = ref(false)
    const router = useRouter()
    const auth = useAuthStore()
    const toast = useToast()

    async function handleRegister() {
    // Валидация
    if (!login.value.trim() || !password.value || !passwordConfirm.value) {
    toast.add({ severity: 'error', summary: 'Ошибка', detail: 'Заполните все поля', life: 3000 });
    return;
    }
    if (password.value !== passwordConfirm.value) {
    toast.add({ severity: 'error', summary: 'Ошибка', detail: 'Пароли не совпадают', life: 3000 });
    return;
    }
    if (password.value.length < 4) {
    toast.add({ severity: 'error', summary: 'Ошибка', detail: 'Пароль должен быть не менее 4 символов', life: 3000 });
    return;
    }

    loading.value = true;
    try {
    // Хешируем пароль
    const hashBuffer = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(password.value))
    const hashHex = Array.from(new Uint8Array(hashBuffer))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('')

    const response = await fetch('/api/register', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
    login: login.value.trim(),
    password_hash: hashHex
    })
    })

    const data = await response.json()
    if (!response.ok) {
    throw new Error(data.error || 'Ошибка регистрации')
    }

    // Сохраняем токен и ID пользователя
    auth.setAuth(data.token, data.user_id)

    // Перенаправляем в чат
    router.push('/chats')
    } catch (e: any) {
    toast.add({ severity: 'error', summary: 'Ошибка регистрации', detail: e.message, life: 5000 })
    } finally {
    loading.value = false
    }
    }
</script>

<style scoped>
    .auth-wrapper {
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 80vh;
        background: var(--surface-ground);
    }

    .auth-card {
        width: 400px;
    }

    .login-link {
        text-align: center;
        margin-top: 1rem;
    }
</style>