<script setup lang="ts">

</script>

<template>
  <div class="h-full w-full p-4 flex flex-col gap-6">
    <AsyncFetch
      v-slot="{ data, status, refresh }"
      fetch-key="fences"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <TableFences
        :item="data"
        :loading="status === 'pending'"
        @delete-item="async (e) => {
          await $trpc.fences.delete.mutate({ id: e.properties.id })
          await refresh()
        }"
        @refresh-table="refresh"
      />
    </AsyncFetch>
  </div>
</template>
