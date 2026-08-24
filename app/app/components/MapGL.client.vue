<script setup lang="ts" generic="T extends GeoJSON.GeoJsonProperties">
import { useMap } from '@indoorequal/vue-maplibre-gl'

type Props = {
  zoom?: number
  center?: [number, number]
  events?: GeoJSON.FeatureCollection<GeoJSON.Point, T>
  fences?: GeoJSON.FeatureCollection<GeoJSON.Polygon>
}

const props = withDefaults(defineProps<Props>(), {
  zoom: 10,
  center: () => [-51.2300, -30.0346],
  events: () => ({ type: 'FeatureCollection', features: [] }),
  fences: () => ({ type: 'FeatureCollection', features: [] }),
})

type FeatWithoutId = Omit<GeoJSON.Feature<GeoJSON.Polygon>, 'id'>
type FeatWithId = FeatWithoutId & { id: string }

const emits = defineEmits<{
  'fence-created': [fence: FeatWithoutId]
  'fence-updated': [fence: FeatWithId]
  'fence-deleted': [fence: FeatWithId]
}>()

const hovered = ref<GeoJSON.Feature<GeoJSON.Point, Event> | null>(null)

// Fit the map to the events when the map is loaded
const map = useMap()
watch(
  () => map.isLoaded,
  async (loaded) => {
    if (!loaded) return
    if (!map.map) return
    if (!props.events) return
    if (props.events.features.length === 0) return

    const { bbox } = await import('@turf/turf')
    const [minX, minY, maxX, maxY] = bbox(props.events)
    map.map.fitBounds([minX - 0.001, minY - 0.001, maxX + 0.001, maxY + 0.001], { animate: false })
  },
)
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
