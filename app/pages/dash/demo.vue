<script setup lang="ts">
import type { AppRouterOutputs } from '~~/server/trpc'

const { $trpc } = useNuxtApp()

const [
  { data: dataItems, refresh: refreshItems, error: errorItems, status: statusItems },
  { data: dataFiles, refresh: refreshFiles, error: errorFiles, status: statusFiles },
] = await Promise.all([
  $trpc.items.list.useQuery(undefined),
  $trpc.files.list.useQuery(undefined),
])

if (errorItems.value) throw createError({
  status: errorItems.value.data?.httpStatus,
  message: JSON.stringify(errorItems.value.data),
})

if (errorFiles.value) throw createError({
  status: errorFiles.value.data?.httpStatus,
  message: JSON.stringify(errorFiles.value.data),
})

const onSubmitFile = async (e: File) => {
  const data = new FormData()
  data.append('file', e)
  await $trpc.files.create.mutate(data)
  await refreshFiles()
}

type ItemSearch = AppRouterOutputs['vector']['search'][number]

const searchResults = ref<ItemSearch[]>([])
const isSearching = ref(false)

const onSearch = async (e: { query: string, prefix?: string }) => {
  isSearching.value = true
  try {
    const results = await $trpc.vector.search.query(e)
    searchResults.value = results
  }
  finally {
    isSearching.value = false
  }
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
      <UCard class="flex flex-col gap-4">
        <template #header>
          <h2 class="text-2xl font-bold">
            Database
          </h2>
        </template>

        <FormItem
          :loading="statusItems === 'pending'"
          @submit="async (e) => {
            await $trpc.items.create.mutate(e)
            await refreshItems()
          }"
        />

        <TableItems
          :items="dataItems || []"
          :loading="statusItems === 'pending'"
          @refresh-table="async () => {
            await refreshItems()
          }"
          @update-user="async (e) => {
            await $trpc.items.update.mutate(e)
            await refreshItems()
          }"
          @delete-user="async (e) => {
            await $trpc.items.delete.mutate(e)
            await refreshItems()
          }"
          @select-user="async (e) => {
            await $trpc.items.update.mutate(e)
            await refreshItems()
          }"
        />
      </UCard>

      <UCard class="flex flex-col gap-4">
        <template #header>
          <h2 class="text-2xl font-bold">
            Files
          </h2>
        </template>

        <FormFile
          @submit="async (e) => {
            await onSubmitFile(e)
          }"
        />

        <TableFiles
          :items="dataFiles || []"
          :loading="statusFiles === 'pending'"
          @refresh-table="async () => {
            await refreshFiles()
          }"
          @download-file="async (e) => {

          }"
          @delete-file="async (e) => {
            await $trpc.files.delete.mutate({ key: e.key })
            await refreshFiles()
          }"
        />
      </UCard>
    </div>

    <UCard>
      <template #header>
        <h2 class="text-2xl font-bold flex items-center gap-2">
          <UIcon name="i-lucide-brain-circuit" />
          Vector Search
        </h2>
      </template>

      <div class="flex flex-col gap-6">
        <FormSearch @submit="onSearch" />

        <TableVectors
          :items="searchResults"
          :loading="isSearching"
          @refresh-table="() => searchResults = []"
        />
      </div>
    </UCard>
  </div>
</template>
