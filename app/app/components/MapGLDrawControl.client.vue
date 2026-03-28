<script setup lang="ts">
import { useMap } from '@indoorequal/vue-maplibre-gl'
import MapboxDraw, { type DrawCreateEvent, type DrawDeleteEvent, type DrawUpdateEvent } from '@mapbox/mapbox-gl-draw'
import '@mapbox/mapbox-gl-draw/dist/mapbox-gl-draw.css'
import type { IControl, Listener, MapLayerEventType } from 'maplibre-gl'
import { onBeforeUnmount, watch } from 'vue'

// MapLibre-compatible draw styles (arrays must be wrapped in ["literal", ...])
const STYLES_DRAW = [
  { id: 'gl-draw-polygon-fill-inactive', type: 'fill', filter: ['all', ['==', 'active', 'false'], ['==', '$type', 'Polygon'], ['!=', 'mode', 'static']], paint: { 'fill-color': '#3bb2d0', 'fill-outline-color': '#3bb2d0', 'fill-opacity': 0.1 } },
  { id: 'gl-draw-polygon-fill-active', type: 'fill', filter: ['all', ['==', 'active', 'true'], ['==', '$type', 'Polygon']], paint: { 'fill-color': '#fbb03b', 'fill-outline-color': '#fbb03b', 'fill-opacity': 0.1 } },
  { id: 'gl-draw-polygon-midpoint', type: 'circle', filter: ['all', ['==', '$type', 'Point'], ['==', 'meta', 'midpoint']], paint: { 'circle-radius': 3, 'circle-color': '#fbb03b' } },
  { id: 'gl-draw-polygon-stroke-inactive', type: 'line', filter: ['all', ['==', 'active', 'false'], ['==', '$type', 'Polygon'], ['!=', 'mode', 'static']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#3bb2d0', 'line-width': 2 } },
  { id: 'gl-draw-polygon-stroke-active', type: 'line', filter: ['all', ['==', 'active', 'true'], ['==', '$type', 'Polygon']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#fbb03b', 'line-dasharray': ['literal', [0.2, 2]], 'line-width': 2 } },
  { id: 'gl-draw-line-inactive', type: 'line', filter: ['all', ['==', 'active', 'false'], ['==', '$type', 'LineString'], ['!=', 'mode', 'static']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#3bb2d0', 'line-width': 2 } },
  { id: 'gl-draw-line-active', type: 'line', filter: ['all', ['==', '$type', 'LineString'], ['==', 'active', 'true']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#fbb03b', 'line-dasharray': ['literal', [0.2, 2]], 'line-width': 2 } },
  { id: 'gl-draw-polygon-and-line-vertex-stroke-inactive', type: 'circle', filter: ['all', ['==', 'meta', 'vertex'], ['==', '$type', 'Point'], ['!=', 'mode', 'static']], paint: { 'circle-radius': 5, 'circle-color': '#fff' } },
  { id: 'gl-draw-polygon-and-line-vertex-inactive', type: 'circle', filter: ['all', ['==', 'meta', 'vertex'], ['==', '$type', 'Point'], ['!=', 'mode', 'static']], paint: { 'circle-radius': 3, 'circle-color': '#fbb03b' } },
  { id: 'gl-draw-point-point-stroke-inactive', type: 'circle', filter: ['all', ['==', 'active', 'false'], ['==', '$type', 'Point'], ['==', 'meta', 'feature'], ['!=', 'mode', 'static']], paint: { 'circle-radius': 5, 'circle-opacity': 1, 'circle-color': '#fff' } },
  { id: 'gl-draw-point-inactive', type: 'circle', filter: ['all', ['==', 'active', 'false'], ['==', '$type', 'Point'], ['==', 'meta', 'feature'], ['!=', 'mode', 'static']], paint: { 'circle-radius': 3, 'circle-color': '#3bb2d0' } },
  { id: 'gl-draw-point-stroke-active', type: 'circle', filter: ['all', ['==', '$type', 'Point'], ['==', 'active', 'true'], ['!=', 'meta', 'midpoint']], paint: { 'circle-radius': 7, 'circle-color': '#fff' } },
  { id: 'gl-draw-point-active', type: 'circle', filter: ['all', ['==', '$type', 'Point'], ['!=', 'meta', 'midpoint'], ['==', 'active', 'true']], paint: { 'circle-radius': 5, 'circle-color': '#fbb03b' } },
  { id: 'gl-draw-polygon-fill-static', type: 'fill', filter: ['all', ['==', 'mode', 'static'], ['==', '$type', 'Polygon']], paint: { 'fill-color': '#404040', 'fill-outline-color': '#404040', 'fill-opacity': 0.1 } },
  { id: 'gl-draw-polygon-stroke-static', type: 'line', filter: ['all', ['==', 'mode', 'static'], ['==', '$type', 'Polygon']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#404040', 'line-width': 2 } },
  { id: 'gl-draw-line-static', type: 'line', filter: ['all', ['==', 'mode', 'static'], ['==', '$type', 'LineString']], layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': '#404040', 'line-width': 2 } },
  { id: 'gl-draw-point-static', type: 'circle', filter: ['all', ['==', 'mode', 'static'], ['==', '$type', 'Point']], paint: { 'circle-radius': 5, 'circle-color': '#404040' } },
]

// MapLibre compatibility: patch CSS class constants used by MapboxDraw this is
// needed so the controls appear with correct styling in maplibre-gl
//
// eslint-disable-next-line @typescript-eslint/no-explicit-any
const CLASSES_DRAW = MapboxDraw.constants.classes as any
CLASSES_DRAW.CANVAS = 'maplibregl-canvas'
CLASSES_DRAW.CONTROL_BASE = 'maplibregl-ctrl'
CLASSES_DRAW.CONTROL_PREFIX = 'maplibregl-ctrl-'
CLASSES_DRAW.CONTROL_GROUP = 'maplibregl-ctrl-group'
CLASSES_DRAW.ATTRIBUTION = 'maplibregl-ctrl-attrib'

const model = defineModel<GeoJSON.FeatureCollection<GeoJSON.Polygon>>({
  default: () => ({ type: 'FeatureCollection', features: [] }),
})

type FeatWithoutId = Omit<GeoJSON.Feature<GeoJSON.Polygon>, 'id'>
type FeatWithId = FeatWithoutId & { id: string }

const emits = defineEmits<{
  'fence-created': [fence: FeatWithoutId]
  'fence-updated': [fence: FeatWithId]
  'fence-deleted': [fence: FeatWithId]
}>()

const map = useMap()
let draw: InstanceType<typeof MapboxDraw> | null = null

watch(
  () => map.isLoaded,
  (loaded) => {
    if (!loaded) return
    if (!map.map) return

    // Patch map.on for maplibre-gl v5 compatibility with mapbox-gl-draw touch
    // events
    const originalOn = map.map.on.bind(map.map)

    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    map.map.on = (type: keyof MapLayerEventType, handler: any, options?: any) => {
      return originalOn(type, handler, options)
    }

    draw = new MapboxDraw({
      displayControlsDefault: false,
      controls: { polygon: true, trash: true },
      styles: STYLES_DRAW,
    })

    map.map.addControl(draw as unknown as IControl)

    // Load initial features if provided
    if (model.value.features.length) draw.set(model.value)

    const onDrawCreate: Listener = (e: DrawCreateEvent) => {
      if (!draw) return
      if (!e.features[0]) return
      const feature = e.features[0] as GeoJSON.Feature<GeoJSON.Polygon>
      emits('fence-created', feature as FeatWithoutId)
    }

    const onDrawUpdate: Listener = (e: DrawUpdateEvent) => {
      if (!draw) return
      if (!e.features[0]) return
      const feature = e.features[0] as GeoJSON.Feature<GeoJSON.Polygon>
      emits('fence-updated', feature as FeatWithId)
    }

    const onDrawDelete: Listener = (e: DrawDeleteEvent) => {
      if (!draw) return
      if (!e.features[0]) return
      const feature = e.features[0] as GeoJSON.Feature<GeoJSON.Polygon>
      emits('fence-deleted', feature as FeatWithId)
    }

    map.map.on('draw.create', onDrawCreate)
    map.map.on('draw.update', onDrawUpdate)
    map.map.on('draw.delete', onDrawDelete)
  },
)

// Sync prop changes into draw control (external → draw)
watch(
  () => model.value,
  features => draw?.set(features),
  { deep: true },
)

onBeforeUnmount(() => {
  // Let draw control be garbage collected; don't call removeControl since the
  // parent map is already being destroyed
  draw = null
})
</script>

<template>
  <div />
</template>
