<template>
  <div class="chat-window">
    <div v-for="msg in messages" :key="msg.timestamp" class="message">
      <span class="sender">{{ msg.sender }}:</span> {{ msg.text }}
    </div>
  </div>
</template>

<script setup>
defineProps(['messages']);
</script>

<style scoped>
.chat-window { height: 300px; overflow-y: auto; border: 1px solid #ccc; padding: 0.5rem; }
.message { margin-bottom: 0.3rem; }
.sender { font-weight: bold; }
</style>