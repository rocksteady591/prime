<template>
    <div class="main-container">
        <slot name="sidebar" />
        <slot name="main" />
    </div>
</template>

<style scoped>
    .main-container {
        display: flex;
        flex: 1;
        overflow: hidden;
    }
</style>