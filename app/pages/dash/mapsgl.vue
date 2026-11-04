<script setup lang="ts">
const view = ref<'ndvi' | 'rgb'>('ndvi')
</script>

<template>
  <div class="flex flex-col gap-4 h-full w-full">
    <URadioGroup
      v-model="view"
      orientation="horizontal"
      :items="[
        { label: 'NDVI', value: 'ndvi' },
        { label: 'RGB', value: 'rgb' },
      ]"
    />

    <AsyncFetch
      v-slot="{ data: polygonsData, refresh: polygonsRefresh }"
      fetch-key="polygons"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <MapGL
        class="h-full w-full"
        :fences="polygonsData"
        :view="view"
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
  </div>
</template>
