<script setup lang="ts">
definePageMeta({ auth: true })
</script>

<template>
  <div class="flex flex-col gap-4">
    <UCard>
      <div class="flex flex-col gap-6 p-4">
        <div class="flex items-center gap-3">
          <UIcon
            name="i-lucide-settings"
            class="size-6 text-primary"
          />
          <h1 class="text-2xl font-bold">
            Settings
          </h1>
        </div>

        <p class="text-muted">
          Settings page coming soon.
        </p>
      </div>
    </UCard>
  </div>
</template>
