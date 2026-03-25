<script setup lang="ts">
import type { AppRouterOutputs } from '~~/server/trpc'
import { ref } from 'vue'

type Response = AppRouterOutputs['events']['list']
defineProps<{ data: Response }>()

const drawnFeatures = ref<GeoJSON.Feature[]>([])

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
  <MglMap
    width="100%"
    height="100%"
    :center="[-51.2300, -30.0346]"
    :zoom="10"
    :map-style="'https://tiles.openfreemap.org/styles/liberty'"
  >
    <MglDrawControl v-model:features="drawnFeatures" />

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
          // Color ramp: Hue 200 (blue) to 340 (pink); use colorValue property (0..1)
          'circle-color': [
            'interpolate',
            ['linear'],
            ['get', 'colorValue'],
            0, 'hsl(200, 80%, 55%)',
            1, 'hsl(340, 80%, 60%)',
          ],
          'circle-opacity': [
            'interpolate',
            ['linear'],
            ['get', 'colorValue'],
            0, 0.75,
            1, 1,
          ],
        }"
      />
    </MglGeoJsonSource>
  </MglMap>
</template>
