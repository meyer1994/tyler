<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'
import type { AppRouterOutputs } from '~~/server/trpc'

type Item = AppRouterOutputs['files']['list'][number]

type Props = {
  items: Item[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'delete-file': [key: Item]
  'download-file': [key: Item]
  'refresh-table': []
}>()

type Keys = 'key' | 'size' | 'createdAt' | 'actions'

// Table columns
const columns: TableColumn<Item>[] = [
  {
    id: 'key' as const,
    accessorKey: 'key',
    header: 'Nome do Arquivo',
  },
  {
    id: 'size' as const,
    accessorKey: 'size',
    header: 'Tamanho',
  },
  {
    id: 'createdAt' as const,
    accessorKey: 'createdAt',
    header: 'Enviado em',
  },
  {
    id: 'actions' as const,
    header: 'Ações',
  },
]

const MAP_ID_TO_LABEL: Record<string, string> = columns
  .reduce((a, c) => ({ ...a, [c.id as string]: c.header }), {})

// State for column visibility
const visible: Ref<Record<Keys, boolean>> = ref({
  key: true,
  size: true,
  createdAt: true,
  actions: true,
})

const table = useTemplateRef('table')
</script>

<template>
  <div class="flex flex-col flex-1 w-full">
    <!-- Filter and column controls -->
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
        @click="e => emit('refresh-table')"
      />

      <UDropdownMenu
        :items="
          table
            ?.tableApi
            ?.getAllColumns()
            .filter((column) => column.getCanHide())
            .map((column) => ({
              label: (MAP_ID_TO_LABEL[column.id] ?? column.id),
              type: 'checkbox' as const,
              checked: column.getIsVisible(),
              onUpdateChecked: (checked: boolean) =>
                table
                  ?.tableApi
                  ?.getColumn(column.id)
                  ?.toggleVisibility(!!checked),
              // prevents the select menu from closing
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
        icon: 'i-lucide-box',
        label: 'No files found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
    >
      <template #key-cell="{ row }">
        <span class="text-sm font-medium">
          {{ row.original.key || 'N/A' }}
        </span>
      </template>

      <template #size-cell="{ row }">
        <FormatBytes
          :value="row.original.size"
          class="text-sm text-muted"
        />
      </template>

      <template #createdAt-cell="{ row }">
        <NuxtTime
          v-if="row.original?.createdAt"
          :datetime="row.original.createdAt"
          title
          class="text-sm text-muted"
        />
        <span
          v-else
          class="text-sm text-muted"
        >N/A</span>
      </template>

      <!-- Actions Column -->
      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-trash-2"
            color="error"
            variant="ghost"
            size="sm"
            title="Delete file"
            aria-label="Delete file"
            @click="emit('delete-file', row.original)"
          />
          <UButton
            icon="i-lucide-download"
            color="primary"
            variant="ghost"
            size="sm"
            title="Download file"
            aria-label="Download file"
            :to="row.original?.url"
            target="_blank"
            @click="emit('download-file', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
