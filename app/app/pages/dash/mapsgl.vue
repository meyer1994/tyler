<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <MapGL :data="data ?? []" />
  </AsyncFetch>
</template>
