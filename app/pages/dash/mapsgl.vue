<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data: polygonsData, refresh: polygonsRefresh }"
    fetch-key="polygons"
    :handler="async () => await $trpc.fences.list.query()"
  >
    <div class="relative h-full w-full">
      <MapGL
        class="h-full w-full"
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
    </div>
  </AsyncFetch>
</template>
