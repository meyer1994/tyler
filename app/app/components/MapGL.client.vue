<script setup lang="ts" generic="T extends GeoJSON.GeoJsonProperties">
type Props = {
  events?: GeoJSON.FeatureCollection<GeoJSON.Point, T>
  fences?: GeoJSON.FeatureCollection<GeoJSON.Polygon>
}

const props = withDefaults(defineProps<Props>(), {
  events: () => ({ type: 'FeatureCollection', features: [] }),
  fences: () => ({ type: 'FeatureCollection', features: [] }),
})
console.log(props.events)
console.log(props.fences)

const hovered = ref<GeoJSON.Feature<GeoJSON.Point, Event> | null>(null)
</script>

<template>
  <MglMap
    width="100%"
    height="400px"
    :center="[-51.2300, -30.0346]"
    :zoom="10"
    :map-style="'https://tiles.openfreemap.org/styles/liberty'"
  >
    <MapGLDrawControl
      :model-value="props.fences"
      @update:model-value="console.log($event)"
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
</template>
