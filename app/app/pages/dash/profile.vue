<script setup lang="ts">
definePageMeta({ auth: true })

const { user } = useAuth()
</script>

<template>
  <div class="flex flex-col gap-4">
    <UCard v-if="user">
      <div class="flex flex-col items-center gap-6 p-4">
        <UAvatar
          :src="user.image ?? undefined"
          :alt="user.name ?? undefined"
          size="3xl"
          icon="i-lucide-user"
        />

        <div class="text-center">
          <h1 class="text-2xl font-bold">
            {{ user.name ?? undefined }}
          </h1>
          <p class="text-muted">
            {{ user.email ?? undefined }}
          </p>
        </div>

        <div class="w-full space-y-4">
          <div class="flex items-center justify-between rounded-lg bg-elevated p-3">
            <div class="flex items-center gap-2">
              <UIcon
                name="i-lucide-mail"
                class="text-muted"
              />
              <span class="text-sm text-muted">Email verified</span>
            </div>
            <UBadge
              :color="user.emailVerified ? 'success' : 'warning'"
              variant="subtle"
            >
              {{ user.emailVerified ? 'Verified' : 'Not verified' }}
            </UBadge>
          </div>

          <div class="flex items-center justify-between rounded-lg bg-elevated p-3">
            <div class="flex items-center gap-2">
              <UIcon
                name="i-lucide-calendar"
                class="text-muted"
              />
              <span class="text-sm text-muted">Member since</span>
            </div>
            <NuxtTime
              class="text-sm"
              :datetime="user.createdAt"
              title
            />
          </div>
        </div>
      </div>
    </UCard>
  </div>
</template>
