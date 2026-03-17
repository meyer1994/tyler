<script setup lang="ts">
type Props = {
  value: number
}

const props = defineProps<Props>()

const formatBytes = (bytes: number) => {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(2))} ${sizes[i]}`
}

const formatted = computed(() => formatBytes(props.value))
</script>

<template>
  <span>{{ formatted }}</span>
</template>
