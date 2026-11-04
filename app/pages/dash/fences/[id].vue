<script setup lang="ts">
import type { FeatureCollection } from 'geojson'

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

    <div class="flex flex-1 min-h-0 flex-col gap-4 lg:flex-row">
      <AsyncFetch
        v-slot="{ data, status }"
        :fetch-key="`fence-${id}`"
        :handler="async () => await $trpc.fences.get.query({ id })"
      >
        <div class="flex flex-1 min-h-0 flex-col gap-4 lg:flex-row">
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

        <UTabs
          class="w-128"
          :model-value="String($route.query.tab ?? 'geojson')"
          :items="[
            { label: 'Search', value: 'search', slot: 'search' },
            { label: 'GeoJSON', value: 'geojson', slot: 'geojson' },
          ]"
          @update:model-value="navigateTo({ query: { ...$route.query, tab: $event } })"
        >
          <template #search>
            <UCard
              title="Search"
              :ui="{ body: 'flex flex-col gap-4' }"
            >
              <AsyncFetch
                v-slot="{ data: searchData }"
                :fetch-key="`fence-${id}-search`"
                :handler="async () => await $fetch<FeatureCollection>(`${url.origin}/api/tiles/fences/${id}/search`)"
              >
                <template
                  v-for="item in searchData?.features"
                  :key="item.properties.href"
                >
                  <UCard :ui="{ header: 'flex items-center justify-between' }">
                    <template #header>
                      <NuxtTime
                        :datetime="item.properties.datetime"
                        title
                        relative
                      />

                      <span class="text-muted text-xs">
                        {{ item.properties.cloudcover?.toFixed(2) }}% cloud cover
                      </span>
                    </template>
                    <NuxtImg :src="`${url.origin}/api/tiles/fences/${id}/image?href=${encodeURIComponent(item.properties.href)}&view=${view}`" />
                  </UCard>
                </template>
              </AsyncFetch>
            </UCard>
          </template>

          <template #geojson>
            <UCard title="GeoJSON">
              <pre class="text-xs whitespace-pre-wrap break-all">{{ JSON.stringify(data, null, 2) }}</pre>
            </UCard>
          </template>
        </UTabs>
      </AsyncFetch>
    </div>
  </div>
</template>
