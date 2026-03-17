<script setup lang="ts">
import type { NavigationMenuItem } from '@nuxt/ui'

const { loggedIn, signOut } = useAuth()

const loggedInItems = ref<NavigationMenuItem[]>([
  {
    label: 'Home',
    icon: 'i-lucide-home',
    description: 'Go to home',
    to: '/',
  },
  {
    label: 'Dashboard',
    icon: 'i-lucide-layout-dashboard',
    description: 'Access your dashboard',
    to: '/dash',
  },
  {
    label: 'Sign out',
    icon: 'i-lucide-log-out',
    description: 'Sign out of your account',
    onSelect: async () => {
      await signOut()
      await reloadNuxtApp({ path: '/' })
    },
  },
])

const loggedOutItems = ref<NavigationMenuItem[]>([
  {
    label: 'Sign In',
    icon: 'i-lucide-log-in',
    description: 'Sign in to your account',
    to: '/signin',
  },
  {
    label: 'Sign Up',
    icon: 'i-lucide-user-plus',
    description: 'Create a new account',
    to: '/signup',
  },
])

const items = computed(() =>
  loggedIn.value ? loggedInItems.value : loggedOutItems.value,
)
</script>

<template>
  <UHeader :ui="{ root: 'scrollbar-gutter-stable' }">
    <template #title>
      <UButton
        label="flower"
        variant="ghost"
        class="text-2xl font-bold"
        icon="i-lucide-flower"
        @click="async () => { await navigateTo('/') }"
      />
    </template>

    <UNavigationMenu :items="items" />

    <template #right>
      <UColorModeButton title="Toggle Color Mode" />
    </template>

    <template #body>
      <UNavigationMenu
        :items="items"
        orientation="vertical"
        class="-mx-2.5"
      />
    </template>
  </UHeader>
</template>
