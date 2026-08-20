<template>
    <form @submit.prevent="onSubmit">
        <input v-model="text" placeholder="Сообщение..." />
        <button type="submit">Отправить</button>
    </form>
</template>

<script setup>
    import { ref } from 'vue';
    const emit = defineEmits(['send']);
    const text = ref('');

    function onSubmit() {
    const trimmed = text.value.trim();
    if (trimmed) {
    emit('send', trimmed);
    text.value = '';
    }
    }
</script>