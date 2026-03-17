<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'
import type { $Infer } from '~~/app/composables/auth'

type Item = typeof $Infer.Member

type Props = {
  items: Item[]
  filter?: string
  loading?: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'select-item': [item: Item]
  'remove-member': [item: Item]
  'refresh-table': []
}>()

type Keys = 'member' | 'role' | 'actions'

const columns: TableColumn<Item>[] = [
  {
    id: 'member' as const,
    header: 'Member',
  },
  {
    id: 'role' as const,
    accessorKey: 'role',
    header: 'Role',
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
  member: true,
  role: true,
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
        label: 'No members found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(e, row) => emit('select-item', row.original)"
    >
      <template #member-cell="{ row }">
        <div class="flex items-center gap-3">
          <UAvatar
            :alt="row.original.user.name"
            size="sm"
          />
          <div class="flex flex-col">
            <span class="font-medium">{{ row.original.user.name }}</span>
            <span class="text-sm text-muted">{{ row.original.user.email }}</span>
          </div>
        </div>
      </template>

      <template #role-cell="{ row }">
        <UBadge
          :color="row.original.role === 'owner' ? 'primary' : 'neutral'"
          :label="row.original.role"
        />
      </template>

      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-user-minus"
            color="error"
            variant="ghost"
            size="sm"
            :disabled="row.original.role === 'owner'"
            @click.stop="emit('remove-member', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
