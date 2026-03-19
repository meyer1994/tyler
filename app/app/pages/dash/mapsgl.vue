<script setup lang="ts">
import type { AppRouterOutputs } from '~~/server/trpc'

type Response = AppRouterOutputs['events']['list']

const toFeatureCollection = (data?: Response): GeoJSON.FeatureCollection => {
  if (!data) return { type: 'FeatureCollection', features: [] }

  const times = data.map(e => new Date(e.createdAt).getTime())
  const minT = Math.min(...times)
  const maxT = Math.max(...times)
  const range = maxT - minT || 1

  return {
    type: 'FeatureCollection',
    features: data.map(event => ({
      type: 'Feature',
      geometry: { type: 'Point', coordinates: [event.lon, event.lat] },
      properties: {
        id: event.id,
        boardId: event.boardId,
        createdAt: event.createdAt,
        colorValue: (new Date(event.createdAt).getTime() - minT) / range,
      },
    })),
  }
}
</script>

<template>
  <AsyncFetch
    v-slot="{ data }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <ClientOnly>
      <MglMap
        width="100%"
        height="100%"
        :center="[-48.5011, -27.5949]"
        :zoom="10"
        :map-style="'https://tiles.openfreemap.org/styles/liberty'"
      >
        <MglGeoJsonSource
          source-id="events-geojson-source"
          :data="toFeatureCollection(data)"
        >
          <MglCircleLayer
            layer-id="events-circle-layer"
            source="events-geojson-source"
            :paint="{
              'circle-radius': 10,
              'circle-stroke-opacity': 0.8,
              'circle-stroke-width': 1,
            }"
          />
        </MglGeoJsonSource>
      </MglMap>
    </ClientOnly>
  </AsyncFetch>
</template>
