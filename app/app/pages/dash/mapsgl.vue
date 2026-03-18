<script setup lang="ts">
const { $trpc } = useNuxtApp()
const { data: events, error } = await useAsyncData(
  'events-mapgl',
  async () => await $trpc.events.list.query(),
  {
    transform: (data) => {
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
      } satisfies GeoJSON.FeatureCollection
    },
  },
)

if (error.value) throw createError(error.value)
</script>

<template>
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
        :data="events ?? { type: 'FeatureCollection', features: [] }"
      >
        <MglCircleLayer
          layer-id="events-circle-layer"
          source="events-geojson-source"
          :paint="{
            'circle-color': [
              'interpolate',
              ['linear'],
              ['get', 'colorValue'],
              0,
              '#2563eb',
              1,
              '#dc2626',
            ],
            'circle-radius': 10,
            'circle-stroke-opacity': 0.8,
            'circle-stroke-width': 1,
          }"
        />
      </MglGeoJsonSource>
    </MglMap>
  </ClientOnly>
</template>
