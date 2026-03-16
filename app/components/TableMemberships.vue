<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'

type Membership = {
  id: string
  role: string
  organization: {
    id: string
    name: string
    slug: string
  }
}

type Props = {
  items: Membership[]
  activeOrgId?: string
  filter?: string
  loading?: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'select-item': [item: Membership]
  'leave-org': [item: Membership]
  'refresh-table': []
}>()

type Keys = 'organization' | 'role' | 'status' | 'actions'

const columns: TableColumn<Membership>[] = [
  {
    id: 'organization' as const,
    header: 'Organization',
  },
  {
    id: 'role' as const,
    accessorKey: 'role',
    header: 'Role',
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
  organization: true,
  role: true,
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
        icon: 'i-lucide-users',
        label: 'Not a member of any organizations',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(_, row) => emit('select-item', row.original)"
    >
      <template #organization-cell="{ row }">
        <div class="flex items-center gap-3">
          <UAvatar
            :alt="row.original.organization.name"
            size="sm"
            icon="i-lucide-building-2"
          />
          <div class="flex flex-col">
            <span class="font-medium">{{ row.original.organization.name }}</span>
            <span class="text-sm text-muted font-mono">{{ row.original.organization.slug }}</span>
          </div>
        </div>
      </template>

      <template #role-cell="{ row }">
        <UBadge
          :color="row.original.role === 'owner' ? 'primary' : 'neutral'"
          :label="row.original.role"
        />
      </template>

      <template #status-cell="{ row }">
        <UBadge
          v-if="props.activeOrgId === row.original.organization.id"
          color="success"
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
            :to="`/dash/orgs/${row.original.organization.id}`"
            @click.stop
          />
          <UButton
            icon="i-lucide-log-out"
            color="error"
            variant="ghost"
            size="sm"
            title="Leave organization"
            :disabled="row.original.role === 'owner'"
            @click.stop="emit('leave-org', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
