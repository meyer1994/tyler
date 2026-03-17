<script setup lang="ts">
import type { DropdownMenuItem, NavigationMenuItem } from '@nuxt/ui'

definePageMeta({ auth: true })

const { user, signOut, client: auth } = useAuth()

const profileItems: DropdownMenuItem[][] = [
  [
    {
      label: 'Profile',
      icon: 'i-lucide-user',
      to: '/dash/profile',
    },
    {
      label: 'Settings',
      icon: 'i-lucide-settings',
      to: '/dash/settings',
    },
  ],
  [
    {
      label: 'Billing',
      icon: 'i-lucide-credit-card',
      to: '/dash/billing',
    },
    {
      label: 'Portal',
      icon: 'i-lucide-external-link',
      onSelect: async () => {
        const { data, error } = await auth.subscription.billingPortal()
        if (error) throw error
        await navigateTo(data.url, { external: true })
      },
    },
  ],
  [
    {
      label: 'Sign out',
      icon: 'i-lucide-log-out',
      onSelect: async () => {
        await signOut()
        await reloadNuxtApp({ path: '/' })
      },
    },
  ],
]

const itemsDemo: NavigationMenuItem[] = [
  {
    label: 'Demo',
    icon: 'i-lucide-flask-conical',
    to: '/dash/demo',
  },
  {
    label: 'Organizations',
    icon: 'i-lucide-building-2',
    to: '/dash/orgs',
  },
  {
    label: 'API Keys',
    icon: 'i-lucide-key',
    to: '/dash/keys',
  },
  {
    label: 'Playground',
    icon: 'i-lucide-terminal',
    to: '/dash/playground',
  },
  {
    label: 'Queue',
    icon: 'i-lucide-list-todo',
    to: '/dash/queue',
  },
]
</script>

<template>
  <UDashboardGroup>
    <UDashboardSidebar
      collapsible
      :ui="{ footer: 'border-t border-default' }"
    >
      <template #header="{ collapsed }">
        <NuxtLink
          to="/dash"
          class="font-semibold flex items-center gap-2"
          :block="collapsed"
        >
          <UIcon
            name="i-lucide-flower"
            class="size-6 text-primary"
          />
          <template v-if="!collapsed">
            flower
          </template>
        </NuxtLink>
      </template>

      <template #default="{ collapsed }">
        <UNavigationMenu
          :block="collapsed"
          :collapsed="collapsed"
          :items="itemsDemo"
          orientation="vertical"
        />
      </template>

      <template #footer="{ collapsed }">
        <UDropdownMenu
          :items="profileItems"
          class="w-full"
          icon="i-lucide-user"
          :ui="{ content: 'w-(--reka-dropdown-menu-trigger-width)' }"
          :block="collapsed"
        >
          <UButton
            v-if="collapsed"
            icon="i-lucide-user"
            color="neutral"
            :title="JSON.stringify(user, null, 2)"
            variant="ghost"
            class="w-full"
            aria-label="Profile menu"
          />
          <UButton
            v-else
            :label="user?.name ? user.name : user?.email ? user.email : 'NA'"
            icon="i-lucide-user"
            color="neutral"
            :title="JSON.stringify(user, null, 2)"
            variant="ghost"
            class="w-full"
            aria-label="Profile menu"
          />
        </UDropdownMenu>
      </template>
    </UDashboardSidebar>

    <UDashboardPanel>
      <template #header>
        <UDashboardNavbar title="Dashboard">
          <template #leading>
            <UDashboardSidebarCollapse />
          </template>
        </UDashboardNavbar>
      </template>

      <template #body>
        <NuxtPage />
      </template>
    </UDashboardPanel>
  </UDashboardGroup>
</template>
