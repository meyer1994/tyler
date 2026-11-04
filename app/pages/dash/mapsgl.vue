<script setup lang="ts">
const collections = [
  { label: 'Sentinel-2 Level-2A', value: 'sentinel-2-l2a' },
  { label: 'Sentinel-2 Pre-Collection 1 Level-2A', value: 'sentinel-2-pre-c1-l2a' },
  { label: 'Sentinel-2 Collection 1 Level-2A', value: 'sentinel-2-c1-l2a' },
  { label: 'Sentinel-2 Level-1C', value: 'sentinel-2-l1c' },
  { label: 'Sentinel-1 Level-1C GRD', value: 'sentinel-1-grd' },
  { label: 'Landsat Collection 2 Level-2', value: 'landsat-c2-l2' },
  { label: 'NAIP: National Agriculture Imagery Program', value: 'naip' },
  { label: 'Copernicus DEM GLO-30', value: 'cop-dem-glo-30' },
  { label: 'Copernicus DEM GLO-90', value: 'cop-dem-glo-90' },
]

const views: { label: string, value: 'ndvi' | 'rgb' }[] = [
  { label: 'NDVI', value: 'ndvi' },
  { label: 'RGB', value: 'rgb' },
]

const collection = ref('sentinel-2-l2a')
const view = ref<'ndvi' | 'rgb'>('ndvi')
</script>

<template>
  <div class="flex flex-col gap-4 h-full w-full">
    <div class="flex items-center gap-4">
      <USelect
        v-model="collection"
        :items="collections"
        class="w-52"
      />

      <URadioGroup
        v-model="view"
        orientation="horizontal"
        :items="views"
      />
    </div>

    <AsyncFetch
      v-slot="{ data: polygonsData, refresh: polygonsRefresh }"
      fetch-key="polygons"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <MapGL
        class="h-full w-full"
        :fences="polygonsData"
        :view="view"
        :collection="collection"
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
