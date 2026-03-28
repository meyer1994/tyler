<script setup lang="ts">
const selected = ref<'hdop' | 'sat' | 'speed' | 'heading' | 'alt'>('hdop')

const MAP_ID_TO_LABEL: Record<typeof selected.value, string> = {
  hdop: 'HDOP',
  sat: 'Satellites',
  speed: 'Speed',
  heading: 'Heading',
  alt: 'Altitude',
}
</script>

<template>
  <AsyncFetch
    v-slot="{ data, status, refresh }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <div class="h-full w-full p-4 flex flex-col gap-6">
      <UFieldGroup>
        <UButton
          v-for="v, k in MAP_ID_TO_LABEL"
          :key="k"
          :color="selected === k ? 'primary' : 'neutral'"
          :label="v"
          @click="selected = k"
        />
      </UFieldGroup>

      <!-- Chart -->
      <GraphLine
        class="flex-1 max-h-48 lg:max-h-64"
        :data="data ?? { type: 'FeatureCollection', features: [] }"
        :getter-ts="f => f.properties.createdAt"
        :getter-val="f => {
          if (selected === 'hdop') return f.properties.hdop ?? 0
          if (selected === 'sat') return f.properties.sat ?? 0
          if (selected === 'speed') return f.properties.speed ?? 0
          if (selected === 'heading') return f.properties.heading ?? 0
          if (selected === 'alt') return f.properties.alt ?? 0
          return -1
        }"
        :series-name="MAP_ID_TO_LABEL[selected]"
        :y-axis-name="MAP_ID_TO_LABEL[selected]"
      />

      <!-- Events Table -->
      <div class="flex-1 min-h-0">
        <TableEvents
          :item="data"
          :loading="status === 'pending'"
          @refresh-table="refresh"
        />
      </div>
    </div>
  </AsyncFetch>
</template>
