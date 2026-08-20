<template>
    <nav class="navbar">
        <router-link to="/chats" class="nav-item" active-class="active">chats</router-link>
        <router-link to="/contacts" class="nav-item" active-class="active">contacts</router-link>
        <router-link to="/settings" class="nav-item" active-class="active">settings</router-link>
        <router-link to="/account" class="nav-item" active-class="active">account</router-link>
        <a v-if="auth.token" class="nav-item logout" @click="handleLogout">выйти</a>
    </nav>
</template>

<script setup lang="ts">
    import { useRouter } from 'vue-router'
    import { useAuthStore } from '@/stores/auth'

    const router = useRouter()
    const auth = useAuthStore()

    function handleLogout() {
    auth.logout()
    router.push('/login').then(() => {
    window.location.reload()
    })
    }
</script>

<style scoped>
    .navbar {
        display: flex;
        background-color: #ffffff;
        border-bottom: 1px solid #e0e0e0;
        box-shadow: 0 2px 4px rgba(0,0,0,0.05);
    }

    .nav-item {
        flex: 1;
        padding: 15px 20px;
        text-align: center;
        font-weight: 600;
        color: #555;
        cursor: pointer;
        transition: background 0.2s, color 0.2s;
        text-transform: capitalize;
        text-decoration: none;
    }

        .nav-item:hover {
            background-color: #f5f5f5;
            color: #007bff;
        }

        .nav-item.active {
            color: #007bff;
            border-bottom: 3px solid #007bff;
            background-color: #f8f9fa;
        }

    .logout {
        color: #dc3545;
    }

        .logout:hover {
            background-color: #fff0f0;
        }
</style>