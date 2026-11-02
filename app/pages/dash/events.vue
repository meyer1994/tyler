<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['events']['list']
type Row = Item['features'][number]
type Keys = keyof Row['properties']

const { $trpc } = useNuxtApp()
const rangeMax = Date.now()
const rangeMin = rangeMax - 30 * 60 * 1000
const range = ref<[number, number]>([rangeMin, rangeMax])
const debouncedRange = useDebounce(range, 300)
const selected = ref<'hdop' | 'sat' | 'speed' | 'heading' | 'alt'>('hdop')

const MAP_ID_TO_LABEL: Record<typeof selected.value, string> = {
  hdop: 'HDOP',
  sat: 'Satellites',
  speed: 'Speed',
  heading: 'Heading',
  alt: 'Altitude',
}

const columns: TableColumn<Row>[] = [
  { id: 'boardId', accessorKey: 'properties.boardId', header: 'Board ID' },
  { id: 'lat', accessorKey: 'properties.lat', header: 'Latitude' },
  { id: 'lon', accessorKey: 'properties.lon', header: 'Longitude' },
  { id: 'alt', accessorKey: 'properties.alt', header: 'Altitude' },
  { id: 'speed', accessorKey: 'properties.speed', header: 'Speed' },
  { id: 'heading', accessorKey: 'properties.heading', header: 'Heading' },
  { id: 'hdop', accessorKey: 'properties.hdop', header: 'HDOP' },
  { id: 'sat', accessorKey: 'properties.sat', header: 'Satellites' },
  { id: 'ts', accessorKey: 'properties.ts', header: 'Timestamp' },
  { id: 'createdAt', accessorKey: 'properties.createdAt', header: 'Created At' },
  { id: 'updatedAt', accessorKey: 'properties.updatedAt', header: 'Updated At' },
]

const MAP_ID_TO_COLUMN_LABEL: Record<Keys, string> = columns.reduce(
  (labels, column) => ({ ...labels, [column.id as Keys]: column.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  id: false,
  boardId: true,
  lat: true,
  lon: true,
  alt: true,
  speed: true,
  heading: false,
  hdop: true,
  sat: true,
  ts: false,
  createdAt: true,
  updatedAt: false,
})

const table = useTemplateRef('table')
</script>

<template>
  <AsyncFetch
    v-slot="{ data, status, refresh }"
    :fetch-key="['events-list', debouncedRange.join(',')]"
    :handler="async () => await $trpc.events.list.query({
      after: new Date(debouncedRange[0]).toISOString(),
      before: new Date(debouncedRange[1]).toISOString(),
    })"
  >
    <div class="h-full w-full p-4 flex flex-col gap-6">
      <EventsTimeSlider
        v-model="range"
        :min="rangeMin"
        :max="rangeMax"
      />

      <UFieldGroup>
        <UButton
          v-for="label, key in MAP_ID_TO_LABEL"
          :key="key"
          :color="selected === key ? 'primary' : 'neutral'"
          :label="label"
          @click="selected = key"
        />
      </UFieldGroup>

      <GraphLine
        class="flex-1 max-h-48 lg:max-h-64"
        :data="data ?? { type: 'FeatureCollection', features: [] }"
        :getter-ts="feature => feature.properties.createdAt"
        :getter-val="feature => {
          if (selected === 'hdop') return feature.properties.hdop ?? 0
          if (selected === 'sat') return feature.properties.sat ?? 0
          if (selected === 'speed') return feature.properties.speed ?? 0
          if (selected === 'heading') return feature.properties.heading ?? 0
          if (selected === 'alt') return feature.properties.alt ?? 0
          return -1
        }"
        :series-name="MAP_ID_TO_LABEL[selected]"
        :y-axis-name="MAP_ID_TO_LABEL[selected]"
      />

      <div class="flex-1 min-h-0 flex flex-col w-full">
        <div class="flex items-center gap-2 py-3.5 border-b border-accented">
          <div class="text-sm text-gray-500">
            Last item @
            <NuxtTime
              v-if="data?.features[0]?.properties.createdAt"
              :datetime="data.features[0].properties.createdAt"
              title
              relative
            />
            <span v-else>—</span>
          </div>

          <UButton
            icon="i-lucide-refresh-cw"
            color="neutral"
            variant="ghost"
            size="sm"
            class="ml-auto"
            @click="refresh()"
          />

          <UDropdownMenu
            :items="
              table?.tableApi
                ?.getAllColumns()
                .filter(column => column.getCanHide())
                .map(column => ({
                  label: MAP_ID_TO_COLUMN_LABEL[column.id as Keys] ?? column.id,
                  type: 'checkbox' as const,
                  checked: column.getIsVisible(),
                  onUpdateChecked: (checked: boolean) =>
                    table?.tableApi?.getColumn(column.id)?.toggleVisibility(!!checked),
                  onSelect: (event: Event) => event.preventDefault(),
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
          :loading="status === 'pending'"
          loading-animation="carousel"
          :columns="columns"
          :data="data?.features ?? []"
          :sticky="true"
          :empty-state="{
            icon: 'i-lucide-map-pin-off',
            label: 'No events found',
          }"
          :ui="{
            tr: 'hover:bg-muted cursor-pointer',
          }"
          class="flex-1"
        >
          <template #boardId-cell="{ row }">
            <span class="font-mono text-sm text-primary font-medium">
              {{ row.original.properties.boardId ?? '-' }}
            </span>
          </template>

          <template #lat-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.lat.toFixed(6) }}
            </span>
          </template>

          <template #lon-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.lon.toFixed(6) }}
            </span>
          </template>

          <template #alt-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.alt?.toFixed(1) ?? '-' }}
            </span>
          </template>

          <template #speed-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.speed?.toFixed(1) ?? '-' }}
            </span>
          </template>

          <template #heading-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.heading?.toFixed(0) ?? '-' }}
            </span>
          </template>

          <template #hdop-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.hdop?.toFixed(2) ?? '-' }}
            </span>
          </template>

          <template #sat-cell="{ row }">
            <span class="font-mono text-sm font-medium">
              {{ row.original.properties.sat ?? '-' }}
            </span>
          </template>

          <template #ts-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.ts ?? '-' }}
            </span>
          </template>

          <template #createdAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.properties.createdAt"
              title
            />
          </template>

          <template #updatedAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.properties.updatedAt"
              title
            />
          </template>
        </UTable>
      </div>
    </div>
  </AsyncFetch>
</template>
