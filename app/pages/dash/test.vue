<script setup lang="ts">
</script>

<template>
  <AsyncFetch
    v-slot="{ data: polygonsData }"
    fetch-key="polygons"
    :handler="async () => await $trpc.fences.list.query()"
  >
    <div class="relative h-full w-full">
      <pre
        class="h-full w-full"
        v-text="polygonsData"
      />
    </div>
  </AsyncFetch>
</template>
