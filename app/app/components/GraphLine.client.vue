<script setup lang="ts">
import type { EChartsOption } from 'echarts'
import { LineChart } from 'echarts/charts'
import { GridComponent, TitleComponent, TooltipComponent } from 'echarts/components'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import type { AppRouterOutputs } from '~~/server/trpc'

type Item = AppRouterOutputs['events']['list'][number]
const props = defineProps<{ items: Item[] }>()

use([
  LineChart,
  GridComponent,
  TitleComponent,
  TooltipComponent,
  CanvasRenderer,
])

const options = computed<EChartsOption>(() => ({
  grid: {
    top: 10, // Minimal spacing at top
    bottom: 20, // Small space for X-axis labels
    left: 10, // Minimal spacing at left
    right: 10, // Minimal spacing at right
    containLabel: true, // Essential: keeps axis labels inside the div
  },
  tooltip: {
    trigger: 'axis',
    axisPointer: {
      type: 'cross',
    },
  },
  xAxis: {
    type: 'time',
  },
  yAxis: [
    {
      type: 'value',
      name: 'HDOP',
      position: 'left',
      splitLine: { show: false },
    },
    {
      type: 'value',
      name: 'Satellites',
      position: 'right',
      splitLine: { show: false },
    },
  ],
  series: [
    {
      name: 'HDOP',
      type: 'line',
      smooth: true,
      symbol: 'none',
      yAxisIndex: 0,
      data: props.items.map(i => [i.createdAt, i.hdop]),
    },
    {
      name: 'Satellites',
      type: 'line',
      smooth: true,
      symbol: 'none',
      yAxisIndex: 1,
      data: props.items.map(i => [i.createdAt, i.sat]),
    },
  ],
}))
</script>

<template>
  <VChart
    :option="options"
    autoresize
  />
</template>
