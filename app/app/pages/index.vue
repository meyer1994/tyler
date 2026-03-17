<script setup lang="ts">
const { loggedIn } = useAuth()

const heroLinks = computed(() => {
  if (loggedIn.value) {
    return [
      { label: 'Go to Dashboard', to: '/dash', icon: 'i-lucide-layout-dashboard', color: 'primary' as const },
    ]
  }
  return [
    { label: 'Sign In', to: '/signin', icon: 'i-lucide-log-in', color: 'primary' as const },
  ]
})
</script>

<template>
  <div class="flex flex-col">
    <NavHeader />

    <UPageHero
      title="trackows"
      description="Track your stuff."
      :links="heroLinks"
    />
  </div>
</template>
