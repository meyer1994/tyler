<script setup lang="ts">
const { $trpc } = useNuxtApp()
const rangeMax = Date.now()
const rangeMin = rangeMax - 30 * 60 * 1000
const range = ref<[number, number]>([rangeMin, rangeMax])
const debouncedRange = useDebounce(range, 300)
</script>

<template>
  <AsyncFetch
    v-slot="{ data: eventsData }"
    :fetch-key="['events-mapgl', debouncedRange.join(',')]"
    :handler="async () => await $trpc.events.list.query({
      after: new Date(debouncedRange[0]).toISOString(),
      before: new Date(debouncedRange[1]).toISOString(),
    })"
  >
    <AsyncFetch
      v-slot="{ data: polygonsData, refresh: polygonsRefresh }"
      fetch-key="polygons"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <div class="relative h-full w-full">
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

        <EventsTimeSlider
          v-model="range"
          :min="rangeMin"
          :max="rangeMax"
          class="absolute inset-x-4 bottom-4 z-10"
        />
      </div>
    </AsyncFetch>
  </AsyncFetch>
</template>
