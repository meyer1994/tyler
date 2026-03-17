<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'
import type { AppRouterOutputs } from '~~/server/trpc'

type Item = AppRouterOutputs['vector']['search'][number]

type Props = {
  items: Item[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'refresh-table': []
}>()

type Keys = 'pageContent' | 'key' | 'indexedAt' | 'score'

const columns: TableColumn<Item>[] = [
  {
    id: 'score' as const,
    accessorKey: 'score',
    header: 'Score',
  },
  {
    id: 'pageContent' as const,
    accessorKey: 'pageContent',
    header: 'Conteúdo',
  },
  {
    id: 'key' as const,
    accessorKey: 'metadata.key',
    header: 'Arquivo',
  },
  {
    id: 'indexedAt' as const,
    accessorKey: 'metadata.indexedAt',
    header: 'Indexado em',
  },
]

const MAP_ID_TO_LABEL: Record<string, string> = {
  score: 'Score',
  pageContent: 'Conteúdo',
  key: 'Arquivo',
  indexedAt: 'Indexado em',
}

const visible: Ref<Record<Keys, boolean>> = ref({
  score: true,
  pageContent: true,
  key: true,
  indexedAt: true,
})

const table = useTemplateRef('table')
</script>

<template>
  <div class="flex flex-col flex-1 w-full">
    <div class="flex items-center gap-2 py-3.5 border-b border-accented">
      <slot name="top" />

      <UButton
        icon="i-lucide-refresh-cw"
        color="neutral"
        variant="ghost"
        size="sm"
        title="Atualizar"
        aria-label="Atualizar"
        class="ml-auto"
        @click="emit('refresh-table')"
      />

      <UDropdownMenu
        :items="
          table
            ?.tableApi
            ?.getAllColumns()
            .filter((column) => column.getCanHide())
            .map((column) => ({
              label: (MAP_ID_TO_LABEL[column.id as Keys] ?? column.id),
              type: 'checkbox' as const,
              checked: column.getIsVisible(),
              onUpdateChecked: (checked: boolean) =>
                table
                  ?.tableApi
                  ?.getColumn(column.id)
                  ?.toggleVisibility(!!checked),
              onSelect: (e: Event) => e.preventDefault(),
            }))
        "
        :content="{ align: 'end' }"
      >
        <UButton
          label="Colunas"
          color="neutral"
          variant="outline"
          trailing-icon="i-lucide-chevron-down"
          aria-label="Selecionar colunas"
        />
      </UDropdownMenu>
    </div>

    <UTable
      ref="table"
      v-model:column-visibility="visible"
      :loading="props.loading"
      loading-animation="carousel"
      :columns="columns"
      :data="props.items"
      :sticky="true"
      :empty-state="{
        icon: 'i-lucide-search-x',
        label: 'Nenhum resultado encontrado',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
    >
      <template #score-cell="{ row }">
        <UBadge
          v-if="row.original.score !== undefined"
          variant="subtle"
          size="sm"
          color="neutral"
        >
          {{ row.original.score.toFixed(4) }}
        </UBadge>
        <span v-else>-</span>
      </template>

      <template #pageContent-cell="{ row }">
        <span
          class="text-sm line-clamp-2 overflow-hidden"
          :title="row.original.pageContent"
        >
          {{ row.original.pageContent }}
        </span>
      </template>

      <template #key-cell="{ row }">
        <span class="font-mono text-xs">
          {{ row.original.metadata.key }}
        </span>
      </template>

      <template #indexedAt-cell="{ row }">
        <NuxtTime
          v-if="row.original.metadata.indexedAt"
          :datetime="(row.original.metadata.indexedAt as string)"
          title
          class="text-xs"
        />
        <span
          v-else
          class="text-xs text-muted-foreground"
        >N/A</span>
      </template>
    </UTable>
  </div>
</template>
