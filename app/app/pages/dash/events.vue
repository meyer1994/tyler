<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data, status, refresh }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <div class="h-full w-full p-4 flex flex-col gap-6">
      <GraphLine
        class="flex-1 max-h-48 lg:max-h-64"
        :item="data"
      />
      <!-- Events Table -->
      <div class="flex-1 min-h-0">
        <TableEvents
          :item="data"
          :loading="status === 'pending'"
          @refresh-table="refresh"
        />
      </div>
    </div>
  </AsyncFetch>
</template>
