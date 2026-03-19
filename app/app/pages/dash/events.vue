<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data, status, refresh }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <div class="h-full w-full p-4 flex flex-col">
      <TableEvents
        :items="data ?? []"
        :loading="status === 'pending'"
        @refresh-table="refresh"
      />
    </div>
  </AsyncFetch>
</template>
