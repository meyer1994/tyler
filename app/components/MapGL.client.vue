<script setup lang="ts" generic="T extends GeoJSON.GeoJsonProperties">
import { useMap } from '@indoorequal/vue-maplibre-gl'
import type { RasterTileSource } from 'maplibre-gl'

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

type FeatWithId = Omit<GeoJSON.Feature<GeoJSON.Polygon>, 'id'> & { id: string }
type FeatWithoutId = Omit<GeoJSON.Feature<GeoJSON.Polygon>, 'id'>

const emits = defineEmits<{
  'fence-created': [fence: FeatWithoutId]
  'fence-updated': [fence: FeatWithId]
  'fence-deleted': [fence: FeatWithId]
}>()

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
</script>

<template>
  <div>
    <MglMap
      :zoom="props.zoom"
      :center="props.center"
      :map-style="'https://tiles.openfreemap.org/styles/liberty'"
    >
      <MapGLDrawControl
        :model-value="props.fences"
        @fence-created="e => emits('fence-created', e)"
        @fence-updated="e => emits('fence-updated', e)"
        @fence-deleted="e => emits('fence-deleted', e)"
      />

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
