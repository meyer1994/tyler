<script setup lang="ts">
import { LMap, LMarker, LTileLayer } from '@vue-leaflet/vue-leaflet'
import type { PointExpression } from 'leaflet'
import 'leaflet/dist/leaflet.css'

const { $trpc } = useNuxtApp()

const zoom = ref(12)
const center = ref<PointExpression>([-27.5954, -48.5480]) // Florianópolis, Brazil

const { data: events } = await useAsyncData(
  'events',
  async () => await $trpc.events.list.query(),
  { default: () => [], server: false },
)
</script>

<template>
  <div class="h-full w-full">
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
          v-for="event in events"
          :key="event.id"
          :lat-lng="[event.lat, event.lon]"
        />
      </LMap>
    </ClientOnly>
  </div>
</template>

<style>
.dark img.leaflet-tile {
  filter: brightness(0.6) invert(1) contrast(3) hue-rotate(200deg) saturate(0.3) brightness(0.7);
}
</style>
