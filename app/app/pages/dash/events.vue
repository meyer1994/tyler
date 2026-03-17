<script setup lang="ts">
const { $trpc } = useNuxtApp()

const { data, status, refresh } = await useAsyncData(
  'events-table',
  async () => await $trpc.events.list.query(),
  { default: () => [] },
)
</script>

<template>
  <div class="h-full w-full p-4 flex flex-col">
    <TableEvents
      :items="data"
      :loading="status === 'pending'"
      @refresh-table="refresh"
    />
  </div>
</template>
