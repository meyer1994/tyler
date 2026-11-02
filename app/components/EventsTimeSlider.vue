<script setup lang="ts">
const props = withDefaults(defineProps<{ min?: number, max?: number }>(), {
  min: Date.now() - 30 * 60 * 1000,
  max: Date.now(),
})

const model = defineModel<[number, number]>({ required: true })

const isFullRange = computed(() =>
  model.value[0] === props.min && model.value[1] === props.max,
)
</script>

<template>
  <div class="rounded-lg border border-default bg-default/95 p-4 shadow-sm backdrop-blur">
    <div class="mb-3 flex items-center gap-3">
      <div class="min-w-0 flex-1 text-sm">
        <NuxtTime
          :datetime="model[0]"
          year="numeric"
          month="2-digit"
          day="2-digit"
          hour="2-digit"
          minute="2-digit"
          second="2-digit"
          title
          class="font-medium"
        />
        <span class="mx-2 text-muted">to</span>
        <NuxtTime
          :datetime="model[1]"
          year="numeric"
          month="2-digit"
          day="2-digit"
          hour="2-digit"
          minute="2-digit"
          second="2-digit"
          title
          class="font-medium"
        />
      </div>

      <UButton
        label="Reset"
        icon="i-lucide-rotate-ccw"
        color="neutral"
        variant="ghost"
        size="xs"
        :disabled="isFullRange"
        @click="model = [props.min, props.max]"
      />
    </div>

    <USlider
      v-model="model"
      :min="props.min"
      :max="props.max"
      aria-label="Event time range"
    />
  </div>
</template>
