<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data: eventsData }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <AsyncFetch
      v-slot="{ data: polygonsData, refresh: polygonsRefresh }"
      fetch-key="polygons"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <MapGL
        class="h-full w-full"
        :events="eventsData"
        :fences="polygonsData"
        @fence-created="async (i) => {
          await $trpc.fences.create.mutate(i)
          await polygonsRefresh()
        }"
        @fence-updated="async (i) => {
          await $trpc.fences.update.mutate(i)
          await polygonsRefresh()
        }"
        @fence-deleted="async (i) => {
          await $trpc.fences.delete.mutate(i)
          await polygonsRefresh()
        }"
      />
    </AsyncFetch>
  </AsyncFetch>
</template>
