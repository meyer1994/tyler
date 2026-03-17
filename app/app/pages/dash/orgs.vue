<script setup lang="ts">
import type { FormOrgCreateData } from '~~/app/components/FormOrgCreate.vue'
import type { $Infer } from '~~/app/composables/auth'

type Organization = typeof $Infer.Organization

definePageMeta({ auth: true })

const router = useRouter()
const { client } = useAuth()
const toast = useToast()

// Fetch organizations user owns/created
const { data: organizations, refresh: refreshOrgs, status: orgsStatus } = await useAsyncData(
  'organizations',
  async () => {
    const result = await client.organization.list()
    return result.data || []
  },
)

// Fetch active organization ID for highlighting
const { data: activeOrg, refresh: refreshActiveOrg } = await useAsyncData(
  'organizations:active',
  async () => {
    const result = await client.organization.getFullOrganization()
    return result.data
  },
)

// Create organization
const createLoading = ref(false)

async function handleCreate(data: FormOrgCreateData) {
  createLoading.value = true
  try {
    const result = await client.organization.create({ name: data.name, slug: data.slug })
    await refreshOrgs()
    // Navigate to the new organization
    if (result.data?.id) {
      await router.push(`/dash/orgs/${result.data.id}`)
    }
  }
  finally {
    createLoading.value = false
  }
}

// Delete organization
async function handleDeleteOrg(org: Organization) {
  try {
    await client.organization.delete({ organizationId: org.id })
    toast.add({
      title: 'Organization deleted',
      description: `${org.name} has been deleted`,
    })
    await refreshOrgs()
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to delete organization',
      color: 'error',
    })
  }
}

// Leave organization
async function handleLeaveOrg(org: Organization) {
  try {
    await client.organization.leave({ organizationId: org.id })
    toast.add({
      title: 'Left organization',
      description: `You have left ${org.name}`,
    })
    await refreshOrgs()
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to leave organization',
      color: 'error',
    })
  }
}

// Activate organization
async function handleActivateOrg(org: Organization) {
  try {
    await client.organization.setActive({ organizationId: org.id })
    await Promise.all([
      refreshOrgs(),
      refreshActiveOrg(),
    ])
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to activate organization',
      color: 'error',
    })
  }
}

// Navigate to organization details
async function handleSelectOrg(org: Organization) {
  console.log('selecting org', org)
  console.log('selecting org', org)
  console.log('selecting org', org)
  await navigateTo(`/dash/orgs/${org.id}`)
}
</script>

<template>
  <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
    <!-- Header -->
    <UCard class="md:col-span-2">
      <div class="flex flex-col gap-6 p-4">
        <div class="flex items-center gap-3">
          <UIcon
            name="i-lucide-building-2"
            class="size-6 text-primary"
          />
          <h1 class="text-2xl font-bold">
            Organizations
          </h1>
        </div>
        <p class="text-muted">
          Manage your organizations and team members.
        </p>
      </div>
    </UCard>

    <!-- Create Organization -->
    <UCard class="md:col-span-1">
      <template #header>
        <div class="flex items-center gap-2">
          <UIcon
            name="i-lucide-plus"
            class="size-5"
          />
          <span class="font-semibold">Create Organization</span>
        </div>
      </template>

      <FormOrgCreate
        :loading="createLoading"
        @submit="handleCreate"
      />
    </UCard>

    <!-- Your Organizations Table -->
    <UCard class="md:col-span-1">
      <template #header>
        <div class="flex items-center gap-2">
          <UIcon
            name="i-lucide-building-2"
            class="size-5"
          />
          <span class="font-semibold">Your Organizations</span>
        </div>
      </template>

      <TableOrgs
        :items="organizations || []"
        :active-org-id="activeOrg?.id"
        :loading="orgsStatus === 'pending'"
        @select-item="handleActivateOrg"
        @delete-org="handleDeleteOrg"
        @leave-org="handleLeaveOrg"
        @activate-org="handleActivateOrg"
        @refresh-table="refreshOrgs"
        @select-org="handleSelectOrg"
      />
    </UCard>

    <!-- Organization Details Subpage -->
    <NuxtPage class="md:col-span-2" />
  </div>
</template>
