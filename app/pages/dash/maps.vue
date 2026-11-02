<script setup lang="ts">
import { LMap, LMarker, LTileLayer } from '@vue-leaflet/vue-leaflet'
import type { PointExpression } from 'leaflet'
import 'leaflet/dist/leaflet.css'

const zoom = ref(12)
const center = ref<PointExpression>([-27.5954, -48.5480]) // Florianópolis, Brazil
</script>

<template>
  <AsyncFetch
    v-slot="{ data }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <ClientOnly>
      <LMap
        v-model:zoom="zoom"
        v-model:center="center"
        :use-global-leaflet="false"
        class="h-full w-full"
      >
        <LTileLayer
          url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
          :min-zoom="0"
          :max-zoom="20"
        />

        <LMarker
          v-for="event in data?.features ?? []"
          :key="event.id"
          :lat-lng="[event.geometry.coordinates[1]!, event.geometry.coordinates[0]!]"
        />
      </LMap>
    </ClientOnly>
  </AsyncFetch>
</template>

<style>
.dark img.leaflet-tile {
  filter: brightness(0.6) invert(1) contrast(3) hue-rotate(200deg) saturate(0.3) brightness(0.7);
}
</style>
