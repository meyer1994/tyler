<script setup lang="ts">
import type { DropdownMenuItem, NavigationMenuItem } from '@nuxt/ui'

definePageMeta({ auth: true })

const { user, signOut } = useAuth()

const navItems: NavigationMenuItem[] = [
  {
    label: 'Maps',
    icon: 'i-lucide-map',
    to: '/dash/maps',
  },
  {
    label: 'Events',
    icon: 'i-lucide-table',
    to: '/dash/events',
  },
]

const profileItems: DropdownMenuItem[][] = [
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
            trackows
          </template>
        </NuxtLink>
      </template>

      <template #default="{ collapsed }">
        <UNavigationMenu
          :block="collapsed"
          :collapsed="collapsed"
          :items="navItems"
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
