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

const url = useRequestURL()
</script>

<template>
  <div class="flex h-full w-full flex-col gap-4 p-4">
    <h1 class="text-lg font-bold">
      Fence <span class="font-mono">#{{ id.substring(0, 8) }}</span>
    </h1>

    <AsyncFetch
      v-slot="{ data, status }"
      :fetch-key="`fence-${id}`"
      :handler="async () => await $trpc.fences.get.query({ id })"
    >
      <div class="flex flex-1 min-h-0 flex-col gap-4 lg:flex-row">
        <div class="flex-1 min-h-0 min-w-0 overflow-hidden rounded-lg border border-default">
          <MapGL
            v-if="data"
            class="h-full w-full"
            :fences="asCollection(data)"
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
