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
    <AsyncFetch
      v-slot="{ data: statsData }"
      fetch-key="events-stats"
      :handler="async () => await $trpc.events.stats.query()"
    >
      <div class="h-full w-full p-4 flex flex-col gap-6">
        <!-- Stats Cards -->
        <div class="grid grid-cols-2 lg:grid-cols-4 gap-4">
          <UCard>
            <template #header>
              <div class="text-sm font-medium text-gray-500">
                Last 1h
              </div>
            </template>
            <div class="text-3xl font-bold">
              {{ statsData?.last1hCount ?? '-' }}
            </div>
            <p class="text-xs text-gray-500 mt-1">
              captures
            </p>
          </UCard>

          <UCard>
            <template #header>
              <div class="text-sm font-medium text-gray-500">
                Last 24h
              </div>
            </template>
            <div class="text-3xl font-bold">
              {{ statsData?.last24hCount ?? '-' }}
            </div>
            <p class="text-xs text-gray-500 mt-1">
              captures
            </p>
          </UCard>

          <UCard>
            <template #header>
              <div class="text-sm font-medium text-gray-500">
                Avg HDOP
              </div>
            </template>
            <div class="text-3xl font-bold">
              {{ statsData?.avgHdop ?? '-' }}
            </div>
            <p class="text-xs text-gray-500 mt-1">
              dilution of precision
            </p>
          </UCard>

          <UCard>
            <template #header>
              <div class="text-sm font-medium text-gray-500">
                Avg Satellites
              </div>
            </template>
            <div class="text-3xl font-bold">
              {{ statsData?.avgSats ?? '-' }}
            </div>
          </UCard>
        </div>

        <!-- Charts -->
        <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <UCard>
            <template #header>
              <div class="text-sm font-medium">
                Satellites Over Time
              </div>
            </template>
            <VChart
              :option="toLineSats(data ?? [])"
              class="min-h-32 lg:min-h-64"
            />
          </UCard>

          <UCard>
            <template #header>
              <div class="text-sm font-medium">
                HDOP Over Time
              </div>
            </template>
            <VChart
              :option="toLineHdop(data ?? [])"
              class="min-h-32 lg:min-h-64"
            />
          </UCard>
        </div>

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
  </AsyncFetch>
</template>
