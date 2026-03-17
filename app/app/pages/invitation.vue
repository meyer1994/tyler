<script setup lang="ts">
import z from 'zod'

const schema = z.object({ id: z.string().min(1, 'Must be a valid invitation ID') })

definePageMeta({
  auth: true,
  validate: (route) => {
    const { success } = schema.safeParse(route.query)
    if (success) return true
    throw createError({ status: 400, message: 'Invalid invitation ID' })
  },
})

const route = useRoute()
const query = route.query as z.infer<typeof schema>

const { client } = useAuth()

const { data } = await useAsyncData('invitation', async () => {
  const { data, error } = await client.organization.acceptInvitation({ invitationId: query.id })
  if (error) throw error
  return data
})
</script>

<template>
  <div class="min-h-screen flex items-center justify-center p-4">
    <UCard class="w-full max-w-md">
      <div class="flex flex-col gap-6 p-6">
        <div class="flex flex-col items-center gap-4">
          <UIcon
            name="i-lucide-check-circle"
            class="size-12 text-success"
          />
          <h1 class="text-xl font-bold text-center">
            Invitation Accepted!
          </h1>
        </div>

        <div class="flex flex-col gap-3 p-4 rounded-lg bg-muted/50">
          <div class="flex justify-between">
            <span class="text-muted">Organization ID</span>
            <span class="font-medium font-mono text-sm">{{ data?.invitation.organizationId }}</span>
          </div>
          <div class="flex justify-between">
            <span class="text-muted">Role</span>
            <UBadge :label="data?.invitation.role" />
          </div>
          <div class="flex justify-between">
            <span class="text-muted">Email</span>
            <span class="font-medium">{{ data?.invitation.email }}</span>
          </div>
        </div>

        <p class="text-muted text-center text-sm">
          Redirecting to your organizations...
        </p>

        <UButton
          to="/dash/orgs"
          label="Go to Organizations"
          block
        />
      </div>
    </UCard>
  </div>
</template>
