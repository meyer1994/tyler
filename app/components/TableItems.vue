<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['items']['list'][number]

type Props = {
  items: Item[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'update-user': [user: Item]
  'delete-user': [user: Item]
  'select-user': [user: Item]
  'refresh-table': []
}>()

type Keys = keyof Item | 'actions'

// Table columns
const columns: TableColumn<Item>[] = [
  {
    id: 'id' as const,
    accessorKey: 'id',
    header: 'ID',
  },
  {
    id: 'name' as const,
    accessorKey: 'name',
    header: 'Name',
  },
  {
    id: 'createdAt' as const,
    accessorKey: 'createdAt',
    header: 'Created At',
  },
  {
    id: 'updatedAt' as const,
    accessorKey: 'updatedAt',
    header: 'Updated At',
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
  id: true,
  name: true,
  userId: false,
  createdAt: true,
  updatedAt: false,
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

    <!-- Table -->
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
        label: 'No items found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(e, row) => emit('select-user', row.original)"
    >
      <!-- ID Column -->
      <template #id-cell="{ row }">
        <span class="font-bold font-mono text-sm">
          {{ row.original?.id
            ? `#${row.original.id.substring(0, 8)}`
            : 'N/A' }}
        </span>
      </template>

      <!-- Name Column -->
      <template #name-cell="{ row }">
        <span class="text-sm">
          {{ row.original?.name || 'N/A' }}
        </span>
      </template>

      <!-- Created At Column -->
      <template #createdAt-cell="{ row }">
        <NuxtTime
          v-if="row.original?.createdAt"
          :datetime="row.original.createdAt"
          title
        />
        <span v-else>N/A</span>
      </template>

      <!-- Updated At Column -->
      <template #updatedAt-cell="{ row }">
        <NuxtTime
          v-if="row.original?.updatedAt"
          :datetime="row.original.updatedAt"
          title
        />
        <span v-else>N/A</span>
      </template>

      <!-- Actions Column -->
      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-trash-2"
            color="error"
            variant="ghost"
            size="sm"
            title="Delete item"
            aria-label="Delete item"
            @click="emit('delete-user', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
