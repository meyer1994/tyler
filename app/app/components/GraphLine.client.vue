<script setup lang="ts" generic="T extends GeoJSON.GeoJsonProperties">
import { LineChart } from 'echarts/charts'
import { GridComponent, TitleComponent, TooltipComponent } from 'echarts/components'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'

type Props = {
  data: GeoJSON.FeatureCollection<GeoJSON.Point, T>
  getterTs: (feature: GeoJSON.Feature<GeoJSON.Point, T>) => number | string
  getterVal: (feature: GeoJSON.Feature<GeoJSON.Point, T>) => number
  seriesName: string
  yAxisName: string
}

const props = defineProps<Props>()
const initOptions = { renderer: 'canvas' as const }

use([
  LineChart,
  GridComponent,
  TitleComponent,
  TooltipComponent,
  CanvasRenderer,
])

const options = computed<ECOption>(() => ({
  grid: {
    top: 10, // Minimal spacing at top
    bottom: 20, // Small space for X-axis labels
    left: 10, // Minimal spacing at left
    right: 10, // Minimal spacing at right
    outerBoundsMode: 'same',
    outerBoundsContain: 'axisLabel',
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
  yAxis: {
    type: 'value',
    name: props.yAxisName,
    position: 'left',
    splitLine: { show: false },
  },
  series: [
    {
      name: props.seriesName,
      type: 'line',
      symbol: 'none',
      data: props.data.features.map(i => [props.getterTs(i), props.getterVal(i)]),
    },
  ],
}))
</script>

<template>
  <VChart
    :option="options"
    :init-options="initOptions"
    autoresize
  />
</template>
