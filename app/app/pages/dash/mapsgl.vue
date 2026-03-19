<script setup lang="ts">
const KEY = computed(() => `events-${crypto.randomUUID()}`)
</script>

<template>
  <AsyncFetch
    v-slot="{ data }"
    :fetch-key="KEY"
    :handler="async () => await $trpc.events.list.query()"
  >
    <MapGL :data="data ?? []" />
  </AsyncFetch>
</template>
