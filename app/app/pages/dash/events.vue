<script setup lang="ts">
import type { EChartsOption } from 'echarts'
import { LineChart } from 'echarts/charts'
import { GridComponent, LegendComponent, TooltipComponent } from 'echarts/components'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import type { AppRouterOutputs } from '~~/server/trpc'

type Item = AppRouterOutputs['events']['list'][number]

use([
  LineChart,
  GridComponent,
  TooltipComponent,
  LegendComponent,
  CanvasRenderer,
])

const toLineSats = (data: Item[]): EChartsOption => {
  return {
    tooltip: { trigger: 'axis' },
    grid: { left: 0, right: 0, top: 0, bottom: 0 },
    xAxis: { type: 'time' },
    yAxis: { type: 'value' },
    series: [
      {
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: data.map(e => [new Date(e.createdAt).getTime(), e.sat]),
      },
    ],
  }
}

const toLineHdop = (data: Item[]): EChartsOption => {
  return {
    tooltip: { trigger: 'axis' },
    grid: { left: 0, right: 0, top: 0, bottom: 0 },
    xAxis: { type: 'time' },
    yAxis: { type: 'value' },
    series: [
      {
        type: 'line',
        smooth: true,
        showSymbol: false,
        data: data.map(e => [new Date(e.createdAt).getTime(), e.hdop]),
      },
    ],
  }
}
</script>

<template>
  <AsyncFetch
    v-slot="{ data, status, refresh }"
    fetch-key="events"
    :handler="async () => await $trpc.events.list.query()"
  >
    <div class="h-full w-full p-4 flex flex-col gap-6">
      <!-- Events Table -->
      <div class="flex-1 min-h-0">
        <TableEvents
          :items="data ?? []"
          :loading="status === 'pending'"
          @refresh-table="refresh"
        />
      </div>
    </div>
  </AsyncFetch>
</template>
