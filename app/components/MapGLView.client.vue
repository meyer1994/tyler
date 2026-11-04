<script setup lang="ts" generic="T extends GeoJSON.GeoJsonProperties">
import { useMap } from '@indoorequal/vue-maplibre-gl'
import type { MapMouseEvent, RasterTileSource } from 'maplibre-gl'

type Props = {
  zoom?: number
  center?: [number, number]
  events?: GeoJSON.FeatureCollection<GeoJSON.Point, T>
  fences?: GeoJSON.FeatureCollection<GeoJSON.Polygon>
  view?: 'ndvi' | 'rgb'
  collection?: string
  // Raster tile URL template (must contain {x}/{y}/{z}). Defaults to the
  // app's own /api/tiles proxy.
  tiles?: string
}

const props = withDefaults(defineProps<Props>(), {
  zoom: 10,
  center: () => [-51.2300, -30.0346],
  events: () => ({ type: 'FeatureCollection', features: [] }),
  fences: () => ({ type: 'FeatureCollection', features: [] }),
  view: 'ndvi',
  collection: 'sentinel-2-l2a',
})

const hovered = ref<GeoJSON.Feature<GeoJSON.Point, Event> | null>(null)

// Fit the map to the events, falling back to the fences, when the map is
// loaded or the data changes
const map = useMap()
const fitToData = async () => {
  if (!map.isLoaded || !map.map) return

  const collection = props.events.features.length > 0 ? props.events : props.fences
  if (collection.features.length === 0) return

  const { bbox } = await import('@turf/turf')
  const [minX, minY, maxX, maxY] = bbox(collection)
  map.map.fitBounds([minX - 0.001, minY - 0.001, maxX + 0.001, maxY + 0.001], { animate: false })
}

watch(
  [() => map.isLoaded, () => props.events, () => props.fences],
  fitToData,
)

const url = useRequestURL()
const tilesUrl = computed(() => {
  const tiles = props.tiles ?? `${url.origin}/api/tiles/{x}/{y}/{z}`
  const separator = tiles.includes('?') ? '&' : '?'
  const query = new URLSearchParams({ view: props.view, collection: props.collection })
  return `${tiles}${separator}${query.toString()}`
})

// The source is only added on load, so refresh its tiles when the URL changes
watch(tilesUrl, (next) => {
  const source = map.map?.getSource('sentinel-raster-source')
  if (source) (source as RasterTileSource).setTiles([next])
})

// Track the pointer so a circle can follow it around the map
const mouse = ref<[number, number] | null>(null)

const onMouseMove = (event: MapMouseEvent) => {
  mouse.value = [event.lngLat.lng, event.lngLat.lat]
}

const onMouseOut = () => {
  mouse.value = null
}

watch(
  () => map.isLoaded,
  (loaded, _, onCleanup) => {
    if (!loaded || !map.map) return
    map.map.on('mousemove', onMouseMove)
    map.map.on('mouseout', onMouseOut)
    onCleanup(() => map.map?.off('mousemove', onMouseMove))
    onCleanup(() => map.map?.off('mouseout', onMouseOut))
  },
)

onBeforeUnmount(() => {
  map.map?.off('mousemove', onMouseMove)
  map.map?.off('mouseout', onMouseOut)
})

const mouseData = computed<GeoJSON.FeatureCollection<GeoJSON.Point>>(() => ({
  type: 'FeatureCollection',
  features: mouse.value
    ? [{ type: 'Feature', geometry: { type: 'Point', coordinates: mouse.value }, properties: {} }]
    : [],
}))
</script>

<template>
  <div>
    <MglMap
      :zoom="props.zoom"
      :center="props.center"
      :map-style="'https://tiles.openfreemap.org/styles/liberty'"
    >
      <MglPopup
        v-if="hovered !== null"
        :coordinates="(hovered.geometry.coordinates as [number, number])"
      >
        <pre class="text-black overflow-x-auto">{{ hovered.properties }}</pre>
      </MglPopup>

      <MglRasterSource
        source-id="sentinel-raster-source"
        :tiles="[tilesUrl]"
        :tile-size="256"
        attribution="Sentinel-2 L2A via TiTiler"
      >
        <MglRasterLayer
          layer-id="sentinel-raster-layer"
          source="sentinel-raster-source"
          :paint="{ 'raster-opacity': 0.7 }"
        />
      </MglRasterSource>

      <MglGeoJsonSource
        source-id="fences-geojson-source"
        :data="props.fences"
      >
        <MglFillLayer
          layer-id="fences-fill-layer"
          source="fences-geojson-source"
          :paint="{
            'fill-color': '#3bb2d0',
            'fill-opacity': 0.15,
          }"
        />
        <MglLineLayer
          layer-id="fences-line-layer"
          source="fences-geojson-source"
          :paint="{
            'line-color': '#3bb2d0',
            'line-width': 2,
          }"
        />
      </MglGeoJsonSource>

      <MglGeoJsonSource
        source-id="mouse-geojson-source"
        :data="mouseData"
      >
        <MglCircleLayer
          layer-id="mouse-circle-layer"
          source="mouse-geojson-source"
          :paint="{
            'circle-radius': 20,
            'circle-color': '#00000000',
            'circle-stroke-color': '#3bb2d0',
            'circle-stroke-width': 2,
          }"
        />
      </MglGeoJsonSource>

      <MglGeoJsonSource
        source-id="events-geojson-source"
        :data="props.events"
      >
        <MglCircleLayer
          layer-id="events-circle-layer"
          source="events-geojson-source"
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
          @mouseenter="hovered = $event.features[0]"
          @mouseleave="hovered = null"
        />
      </MglGeoJsonSource>
    </MglMap>
  </div>
</template>
