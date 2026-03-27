<script setup lang="ts">
import { ref } from 'vue';
import type { AppRouterOutputs } from '~~/server/trpc';

type Item = AppRouterOutputs['events']['list'][number]
defineProps<{ data: Item[] }>()

const drawn = ref<GeoJSON.Feature<GeoJSON.Polygon>[]>([])
const hovered = ref<GeoJSON.Feature<GeoJSON.Point, Item> | null>(null)

const toFeatureCollection = (data?: Item[]): GeoJSON.FeatureCollection => {
  if (!data) return { type: 'FeatureCollection', features: [] }

  // Calculate createdAt color range
  const times = data.map(e => new Date(e.createdAt).getTime())
  const minT = Math.min(...times)
  const maxT = Math.max(...times)
  const rangeT = maxT - minT || 1

  // Calculate value ranges for sat and hdop
  const sats = data.map(e => e.sat ?? 0)
  const minSat = Math.min(...sats)
  const maxSat = Math.max(...sats)
  const rangeSat = maxSat - minSat || 1

  const hdops = data.map(e => e.hdop ?? 0)
  const minHdop = Math.min(...hdops)
  const maxHdop = Math.max(...hdops)
  const rangeHdop = maxHdop - minHdop || 1

  return {
    type: 'FeatureCollection',
    features: data.map(e => ({
      id: e.id,
      type: 'Feature',
      geometry: { type: 'Point', coordinates: [e.lon, e.lat] },
      properties: {
        ...e,
        id: e.id,
        boardId: e.boardId,
        createdAt: e.createdAt,
        colorSat: ((e.sat ?? minSat) - minSat) / rangeSat,
        colorHdop: ((e.hdop ?? minHdop) - minHdop) / rangeHdop,
        colorCreatedAt: (new Date(e.createdAt).getTime() - minT) / rangeT,
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
    <MglDrawControl v-model:features="drawn" />

    <MglPopup 
      v-if="hovered !== null" 
      :coordinates="(hovered.geometry.coordinates as [number, number])"
    >
      <pre class="text-black overflow-x-auto">{{ hovered.properties }}</pre>
    </MglPopup>

    <MglGeoJsonSource
      source-id="events-geojson-source"
      :data="toFeatureCollection(data)"
    >
      <MglCircleLayer
        layer-id="events-circle-layer"
        source="events-geojson-source"
        @mouseenter="hovered = $event.features[0]"
        @mouseleave="hovered = null"
        :paint="{
          'circle-stroke-width': 1,
          'circle-color': [
            'interpolate',
            ['linear'],
            ['get', 'colorCreatedAt'],
            0, 'hsl(200, 80%, 55%)',
            1, 'hsl(340, 80%, 60%)',
          ],
          'circle-radius': [
            'interpolate',
            ['linear'],
            ['get', 'colorSat'],
            0, 6,
            1, 18,
          ],
          'circle-opacity': [
            'interpolate',
            ['linear'],
            ['get', 'hdop'],
            0, 0,
            1, 1,
            2, 0.5,
          ],
        }"
      />
    </MglGeoJsonSource>
  </MglMap>
</template>
