<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <AsyncFetch
      v-slot="{ data: polygons }"
      fetch-key="polygons"
      :handler="async () => await $trpc.events.polygons.query()"
    >
      <MapGL
        :events="data"
        :fences="polygons"
      />
    </AsyncFetch>
  </AsyncFetch>
</template>
