<template>
    <div class="chat-item" :class="{ active }">
        {{ name }}
    </div>
</template>

<script setup lang="ts">
    defineProps<{
    name: string
    active?: boolean
    }>()
</script>

<style scoped>
    .chat-item {
        padding: 15px;
        border-bottom: 1px solid #f0f0f0;
        cursor: pointer;
        font-weight: 500;
        transition: background 0.2s;
    }

        .chat-item:hover {
            background-color: #f0f7ff;
        }

        .chat-item.active {
            background-color: #e1f0ff;
            color: #007bff;
        }
</style>