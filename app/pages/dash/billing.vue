<script setup lang="ts">
definePageMeta({ auth: true })

const url = useRequestURL()
const { user, client: auth } = useAuth()

const { data: subscriptions, status, refresh } = await useAsyncData(
  'subscriptions',
  async () => await auth.subscription.list(),
)
</script>

<template>
  <UCard>
    <template #header>
      <h2 class="text-2xl font-bold">
        {{ subscriptions?.data?.length === 0 ? 'Choose Your Plan' : 'Your Plans' }}
      </h2>
    </template>

    <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
      <UPricingPlan
        title="Starter"
        price="$10/mo"
        description="For small teams and projects"
        :button="{
          label: 'Starter Plan',
          color: 'neutral',
          icon: 'i-lucide-rocket',
          onClick: async () => {
            const { data, error } = await auth.subscription.upgrade({
              plan: 'starter',
              referenceId: user?.id,
              cancelUrl: url.href,
              successUrl: url.href,
              returnUrl: url.href,
            })
            if (error) throw error
            await navigateTo(data.url, { external: true })
          },
        }"
      />

      <UPricingPlan
        title="Pro"
        price="$100/mo"
        description="For large teams and projects"
        :button="{
          label: 'Pro Plan',
          color: 'primary',
          icon: 'i-lucide-crown',
          onClick: async () => {
            const { data, error } = await auth.subscription.upgrade({
              plan: 'pro',
              referenceId: user?.id,
              cancelUrl: url.href,
              successUrl: url.href,
              returnUrl: url.href,
            })
            if (error) throw error
            await navigateTo(data.url, { external: true })
          },
        }"
      />

      <TableSubscriptions
        class="col-span-1 md:col-span-2"
        :items="subscriptions?.data ?? []"
        :loading="status === 'pending'"
        @refresh-table="async () => {
          await refresh()
        }"
        @cancel-item="async (e) => {
          await auth.subscription.cancel({
            returnUrl: url.href,
            referenceId: e.referenceId,
            subscriptionId: e.subscriptionId,
          })
          await refresh()
        }"
      />
    </div>
  </UCard>
</template>
