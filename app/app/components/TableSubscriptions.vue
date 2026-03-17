<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'

type Auth = ReturnType<typeof useAuth>
type Client = Auth['client']
type Item = Awaited<ReturnType<Client['subscription']['list']>>['data'][number]

type Props = {
  items: Item[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'select-item': [item: Item]
  'cancel-item': [item: Item]
  'refresh-table': []
}>()

type Keys = keyof Item | 'actions'

const columns: TableColumn<Item>[] = [
  {
    id: 'id' as const,
    accessorKey: 'id',
    header: 'ID',
  },
  {
    id: 'plan' as const,
    accessorKey: 'plan',
    header: 'Plan',
  },
  {
    id: 'status' as const,
    accessorKey: 'status',
    header: 'Status',
  },
  {
    id: 'stripeSubscriptionId' as const,
    accessorKey: 'stripeSubscriptionId',
    header: 'Stripe ID',
  },
  {
    id: 'periodStart' as const,
    accessorKey: 'periodStart',
    header: 'Period Start',
  },
  {
    id: 'periodEnd' as const,
    accessorKey: 'periodEnd',
    header: 'Period End',
  },
  {
    id: 'cancelAtPeriodEnd' as const,
    accessorKey: 'cancelAtPeriodEnd',
    header: 'Auto-Cancel',
  },
  {
    id: 'seats' as const,
    accessorKey: 'seats',
    header: 'Seats',
  },
  {
    id: 'actions' as const,
    header: 'Actions',
  },
]

const MAP_ID_TO_LABEL: Record<string, string> = columns
  .reduce((a, c) => ({ ...a, [c.id as string]: c.header }), {})

const visible = ref<Record<Keys, boolean>>({
  id: true,
  plan: true,
  referenceId: false,
  stripeCustomerId: false,
  stripeSubscriptionId: false,
  status: true,
  periodStart: true,
  periodEnd: true,
  trialStart: false,
  trialEnd: false,
  cancelAtPeriodEnd: true,
  cancelAt: false,
  canceledAt: false,
  endedAt: false,
  seats: false,
  actions: true,
})

const table = useTemplateRef('table')

// Helper function to get status color
function getStatusColor(status: string | null): 'success' | 'warning' | 'error' | 'primary' | 'neutral' {
  if (!status) return 'neutral'

  switch (status.toLowerCase()) {
    case 'active':
      return 'success'
    case 'incomplete':
    case 'incomplete_expired':
      return 'warning'
    case 'canceled':
    case 'past_due':
    case 'unpaid':
      return 'error'
    case 'trialing':
      return 'primary'
    default:
      return 'neutral'
  }
}
</script>

<template>
  <div class="flex flex-col flex-1 w-full">
    <div class="flex items-center gap-2 py-3.5 border-b border-accented">
      <UButton
        icon="i-lucide-refresh-cw"
        color="neutral"
        variant="ghost"
        size="sm"
        title="Refresh"
        aria-label="Refresh"
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
              label: (MAP_ID_TO_LABEL[column.id] ?? column.id),
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
          label="Columns"
          color="neutral"
          variant="outline"
          trailing-icon="i-lucide-chevron-down"
          aria-label="Select columns"
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
        icon: 'i-lucide-credit-card',
        label: 'No subscriptions found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(e, row) => emit('select-item', row.original)"
    >
      <!-- ID Column -->
      <template #id-cell="{ row }">
        <span class="font-bold font-mono text-sm">
          #{{ row.original.id.substring(0, 8) }}
        </span>
      </template>

      <!-- Plan Column -->
      <template #plan-cell="{ row }">
        <span class="text-sm font-semibold capitalize">
          {{ row.original.plan }}
        </span>
      </template>

      <!-- Status Column -->
      <template #status-cell="{ row }">
        <UBadge
          :color="getStatusColor(row.original.status)"
          variant="subtle"
        >
          {{ row.original.status || 'Unknown' }}
        </UBadge>
      </template>

      <!-- Stripe Subscription ID Column -->
      <template #stripeSubscriptionId-cell="{ row }">
        <code
          v-if="row.original.stripeSubscriptionId"
          class="font-mono text-xs bg-muted px-2 py-1 rounded"
        >
          {{ row.original.stripeSubscriptionId.substring(0, 12) }}...
        </code>
        <span
          v-else
          class="text-muted text-sm"
        >N/A</span>
      </template>

      <!-- Period Start Column -->
      <template #periodStart-cell="{ row }">
        <NuxtTime
          v-if="row.original.periodStart"
          :datetime="row.original.periodStart"
          title
        />
        <span
          v-else
          class="text-muted text-sm"
        >N/A</span>
      </template>

      <!-- Period End Column -->
      <template #periodEnd-cell="{ row }">
        <NuxtTime
          v-if="row.original.periodEnd"
          :datetime="row.original.periodEnd"
          title
        />
        <span
          v-else
          class="text-muted text-sm"
        >N/A</span>
      </template>

      <!-- Cancel At Period End Column -->
      <template #cancelAtPeriodEnd-cell="{ row }">
        <UBadge
          v-if="row.original.cancelAtPeriodEnd"
          color="warning"
          variant="subtle"
        >
          Yes
        </UBadge>
        <span
          v-else
          class="text-muted text-sm"
        >No</span>
      </template>

      <!-- Seats Column -->
      <template #seats-cell="{ row }">
        <span class="text-sm">
          {{ row.original.seats ?? 'N/A' }}
        </span>
      </template>

      <!-- Actions Column -->
      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-eye"
            color="neutral"
            variant="ghost"
            size="sm"
            title="View details"
            aria-label="View details"
            @click.stop="emit('select-item', row.original)"
          />
          <UButton
            icon="i-lucide-trash-2"
            variant="ghost"
            size="sm"
            title="Delete"
            :disabled="row.original.status !== 'active'"
            :color="row.original.status === 'active' ? 'error' : 'neutral'"
            aria-label="Delete"
            @click.stop="emit('cancel-item', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>
