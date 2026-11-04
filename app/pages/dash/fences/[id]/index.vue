<script setup lang="ts">
const route = useRoute()
const { $trpc } = useNuxtApp()

const id = route.params.id as string

const asCollection = (
  feature?: GeoJSON.Feature<GeoJSON.Polygon>,
): GeoJSON.FeatureCollection<GeoJSON.Polygon> => ({
  type: 'FeatureCollection',
  features: feature ? [feature] : [],
})

const views: { label: string, value: 'ndvi' | 'rgb' }[] = [
  { label: 'NDVI', value: 'ndvi' },
  { label: 'RGB', value: 'rgb' },
]

const view = ref<'ndvi' | 'rgb'>('ndvi')

const url = useRequestURL()
</script>

<template>
  <div class="flex h-full w-full flex-col gap-4 p-4">
    <div class="flex items-center justify-between gap-4">
      <h1 class="text-lg font-bold">
        Fence <span class="font-mono">#{{ id.substring(0, 8) }}</span>
      </h1>

      <URadioGroup
        v-model="view"
        orientation="horizontal"
        :items="views"
      />
    </div>

    <AsyncFetch
      v-slot="{ data, status }"
      :fetch-key="`fence-${id}`"
      :handler="async () => await $trpc.fences.get.query({ id })"
    >
      <div class="flex flex-1 min-h-0 flex-col gap-4 lg:flex-row">
        <div class="flex-1 min-h-0 min-w-0 overflow-hidden rounded-lg border border-default">
          <MapGLView
            v-if="data"
            class="h-full w-full"
            :fences="asCollection(data)"
            :view="view"
            :tiles="`${url.origin}/api/tiles/fences/${id}/{x}/{y}/{z}`"
          />
          <div
            v-else
            class="flex h-full items-center justify-center text-muted"
          >
            {{ status === 'pending' ? 'Loading…' : 'Fence not found' }}
          </div>
        </div>

        <div class="shrink-0 overflow-auto rounded-lg border border-default p-4 lg:w-1/3 lg:max-w-md">
          <h2 class="mb-2 font-semibold">
            GeoJSON
          </h2>
          <pre class="text-xs whitespace-pre-wrap break-all">{{ JSON.stringify(data, null, 2) }}</pre>
        </div>
      </div>
    </AsyncFetch>
  </div>
</template>
