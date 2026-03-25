<script setup lang="ts">
import { useMap } from '@indoorequal/vue-maplibre-gl'
import MapboxDraw from '@mapbox/mapbox-gl-draw'
import '@mapbox/mapbox-gl-draw/dist/mapbox-gl-draw.css'
import { onBeforeUnmount, watch } from 'vue'

// MapLibre compatibility: patch CSS class constants used by MapboxDraw
// eslint-disable-next-line @typescript-eslint/no-explicit-any
const drawClasses = MapboxDraw.constants.classes as any
drawClasses.CANVAS = 'maplibregl-canvas'
drawClasses.CONTROL_BASE = 'maplibregl-ctrl'
drawClasses.CONTROL_PREFIX = 'maplibregl-ctrl-'
drawClasses.CONTROL_GROUP = 'maplibregl-ctrl-group'
drawClasses.ATTRIBUTION = 'maplibregl-ctrl-attrib'

const props = withDefaults(
  defineProps<{
    features?: GeoJSON.Feature[]
  }>(),
  { features: () => [] },
)

const emit = defineEmits<{
  'update:features': [features: GeoJSON.Feature[]]
}>()

const mapInstance = useMap()
let draw: InstanceType<typeof MapboxDraw> | null = null

watch(() => mapInstance.isLoaded, (loaded) => {
  if (!loaded || !mapInstance.map) return

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const mapAny = mapInstance.map as any

  // Patch map.on for maplibre-gl v5 compatibility with mapbox-gl-draw touch events
  const originalOn = mapAny.on.bind(mapAny)
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  mapAny.on = (type: string, handler: any, options?: any) => {
    if (typeof handler === 'function' && options !== undefined) {
      return originalOn(type, handler)
    }
    return originalOn(type, handler, options)
  }

  // MapLibre-compatible draw styles (arrays must be wrapped in ["literal", ...])
  const drawStyles = [
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

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  draw = new (MapboxDraw as any)({
    displayControlsDefault: false,
    controls: { polygon: true, trash: true },
    styles: drawStyles,
  })

  mapAny.addControl(draw)

  // Load initial features if provided
  if (props.features.length) {
    draw!.set({ type: 'FeatureCollection', features: props.features })
  }

  mapAny.on('draw.create', onDrawChange)
  mapAny.on('draw.update', onDrawChange)
  mapAny.on('draw.delete', onDrawChange)
})

// Sync prop changes into draw control (external → draw)
watch(
  () => props.features,
  (newFeatures) => {
    if (!draw || !mapInstance.map) return
    draw!.set({ type: 'FeatureCollection', features: newFeatures })
  },
  { deep: true },
)

function onDrawChange() {
  if (!draw) return
  emit('update:features', draw.getAll().features)
}

onBeforeUnmount(() => {
  // Let draw control be garbage collected; don't call removeControl
  // since the parent map is already being destroyed
  draw = null
})
</script>

<!-- eslint-disable-next-line vue/no-template-target-blank -->
<!-- Renderless component: no DOM output, only sets up MapboxDraw control -->
<template>
  <!-- Draw control is added imperatively via useMap() -->
  <div />
</template>
