<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'
import type { $Infer } from '~~/app/composables/auth'

type Item = typeof $Infer.Organization

type Props = {
  items: Item[]
  activeOrgId?: string
  filter?: string
  loading?: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'select-item': [item: Item]
  'delete-org': [item: Item]
  'leave-org': [item: Item]
  'activate-org': [item: Item]
  'select-org': [item: Item]
  'refresh-table': []
}>()

type Keys = 'name' | 'slug' | 'status' | 'actions'

const columns: TableColumn<Item>[] = [
  {
    id: 'name' as const,
    accessorKey: 'name',
    header: 'Name',
  },
  {
    id: 'slug' as const,
    accessorKey: 'slug',
    header: 'Slug',
  },
  {
    id: 'status' as const,
    header: 'Status',
  },
  {
    id: 'actions' as const,
    header: 'Actions',
  },
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (a, c) => ({ ...a, [c.id as Keys]: c.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  name: true,
  slug: true,
  status: true,
  actions: true,
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
        class="ml-auto"
        @click="emit('refresh-table')"
      />

      <UDropdownMenu
        :items="
          table?.tableApi
            ?.getAllColumns()
            .filter((column) => column.getCanHide())
            .map((column) => ({
              label: MAP_ID_TO_LABEL[column.id as Keys] ?? column.id,
              type: 'checkbox' as const,
              checked: column.getIsVisible(),
              onUpdateChecked: (checked: boolean) =>
                table?.tableApi
                  ?.getColumn(column.id)
                  ?.toggleVisibility(!!checked),
              onSelect: (e: Event) => e.preventDefault(),
            }))
        "
        :content="{ align: 'end' }"
      >
        <UButton
          label="Columns"
          color="neutral"
          variant="outline"
          trailing-icon="i-lucide-chevron-down"
        />
      </UDropdownMenu>
    </div>

    <UTable
      ref="table"
      v-model:column-visibility="visible"
      :loading="props.loading"
      loading-animation="carousel"
      :filter="props.filter"
      :global-filter="props.filter"
      :columns="columns"
      :data="props.items"
      :sticky="true"
      :empty-state="{
        icon: 'i-lucide-building-2',
        label: 'No organizations yet',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(_, row) => emit('select-org', row.original)"
    >
      <template #name-cell="{ row }">
        <div class="flex items-center gap-2">
          <UIcon
            name="i-lucide-building-2"
            class="size-4 text-muted"
          />
          <span class="font-medium">{{ row.original.name }}</span>
        </div>
      </template>

      <template #slug-cell="{ row }">
        <span class="font-mono text-sm text-muted">{{ row.original.slug }}</span>
      </template>

      <template #status-cell="{ row }">
        <UBadge
          v-if="props.activeOrgId === row.original.id"
          color="primary"
          label="Active"
        />
        <span
          v-else
          class="text-muted text-sm"
        >-</span>
      </template>

      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-arrow-right"
            color="neutral"
            variant="ghost"
            size="sm"
            :to="`/dash/orgs/${row.original.id}`"
            @click.stop
          />
          <UButton
            icon="i-lucide-check-circle"
            color="primary"
            variant="ghost"
            size="sm"
            title="Activate organization"
            :disabled="props.activeOrgId === row.original.id"
            @click.stop="emit('activate-org', row.original)"
          />
          <UButton
            icon="i-lucide-log-out"
            color="neutral"
            variant="ghost"
            size="sm"
            title="Leave organization"
            @click.stop="emit('leave-org', row.original)"
          />
          <UButton
            icon="i-lucide-trash-2"
            color="error"
            variant="ghost"
            size="sm"
            :disabled="props.activeOrgId === row.original.id"
            @click.stop="emit('delete-org', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
