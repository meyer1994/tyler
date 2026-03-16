<script setup lang="ts">
</script>

<template>
  <div class="flex flex-col gap-6">
    <div class="flex flex-col gap-2">
      <h1 class="text-3xl font-bold">
        Dashboard Overview
      </h1>
      <p class="text-muted">
        Welcome to your dashboard. Select a section from the sidebar to get started.
      </p>
    </div>

    <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
      <UCard
        to="/dash/demo"
        class="hover:bg-elevated transition-colors"
      >
        <div class="flex items-center gap-3">
          <UIcon
            name="i-lucide-flask-conical"
            class="size-8 text-primary"
          />
          <div>
            <h2 class="font-semibold">
              Demo
            </h2>
            <p class="text-sm text-muted">
              Explore demo features
            </p>
          </div>
        </div>
      </UCard>

      <UCard
        to="/dash/keys"
        class="hover:bg-elevated transition-colors"
      >
        <div class="flex items-center gap-3">
          <UIcon
            name="i-lucide-key"
            class="size-8 text-primary"
          />
          <div>
            <h2 class="font-semibold">
              API Keys
            </h2>
            <p class="text-sm text-muted">
              Manage your API keys
            </p>
          </div>
        </div>
      </UCard>

      <UCard
        to="/dash/playground"
        class="hover:bg-elevated transition-colors"
      >
        <div class="flex items-center gap-3">
          <UIcon
            name="i-lucide-terminal"
            class="size-8 text-primary"
          />
          <div>
            <h2 class="font-semibold">
              Playground
            </h2>
            <p class="text-sm text-muted">
              Test and experiment
            </p>
          </div>
        </div>
      </UCard>
    </div>
  </div>
</template>
