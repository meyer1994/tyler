<script setup lang="ts">
import type { FormOrgInviteData } from '~~/app/components/FormOrgInvite.vue'

definePageMeta({ auth: true })

const route = useRoute()
const { client } = useAuth()
const toast = useToast()

const oid = computed(() => route.params.oid as string)

// Set as active and fetch organization details
const { data: org, refresh: refreshOrg, status } = await useAsyncData(
  () => `org-${oid.value}`,
  async () => {
    const query = { organizationId: oid.value }
    const result = await client.organization.getFullOrganization({ query })
    return result.data
  },
)

// Invite member
const inviteLoading = ref(false)

async function handleInvite(data: FormOrgInviteData) {
  inviteLoading.value = true
  try {
    await client.organization.inviteMember({
      email: data.email,
      role: data.role,
      resend: true,
    })
    toast.add({
      title: 'Invitation sent',
      description: `Invitation sent to ${data.email}`,
    })
    await refreshOrg()
  }
  finally {
    inviteLoading.value = false
  }
}

// Remove member
async function handleRemoveMember(member: { id: string, user: { email: string } }) {
  try {
    await client.organization.removeMember({ memberIdOrEmail: member.id })
    toast.add({
      title: 'Member removed',
      description: `${member.user.email} has been removed from the organization`,
    })
    await refreshOrg()
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to remove member',
      color: 'error',
    })
  }
}

// Cancel invitation
async function handleCancelInvitation(invitation: { id: string, email: string }) {
  try {
    await client.organization.cancelInvitation({ invitationId: invitation.id })
    toast.add({
      title: 'Invitation cancelled',
      description: `Invitation to ${invitation.email} has been cancelled`,
    })
    await refreshOrg()
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to cancel invitation',
      color: 'error',
    })
  }
}

// Resend invitation
async function handleResendInvitation(invitation: { id: string, email: string }) {
  try {
    await client.organization.inviteMember({
      email: invitation.email,
      role: 'member',
      resend: true,
    })
    toast.add({
      title: 'Invitation resent',
      description: `Invitation resent to ${invitation.email}`,
    })
  }
  catch {
    toast.add({
      title: 'Error',
      description: 'Failed to resend invitation',
      color: 'error',
    })
  }
}
</script>

<template>
  <UCard v-if="org">
    <template #header>
      <div class="flex items-center gap-2">
        <UIcon
          name="i-lucide-building"
          class="size-5 text-primary"
        />
        <span class="font-semibold">{{ org.name }}</span>
        <UBadge
          color="success"
          label="Active"
          class="ml-auto"
        />
      </div>
    </template>

    <div class="flex flex-col gap-6">
      <!-- Invite Member Form -->
      <div>
        <h3 class="font-medium mb-3">
          Invite Member
        </h3>
        <FormOrgInvite
          :loading="inviteLoading"
          @submit="handleInvite"
        />
      </div>

      <!-- Members Table -->
      <div>
        <h3 class="font-medium mb-3">
          Members ({{ org.members?.length || 0 }})
        </h3>
        <TableOrgMembers
          :items="org.members || []"
          @remove-member="handleRemoveMember"
          @refresh-table="refreshOrg"
        />
      </div>

      <!-- Invitations Table -->
      <div v-if="org.invitations?.length">
        <h3 class="font-medium mb-3">
          Pending Invitations ({{ org.invitations.length }})
        </h3>
        <TableOrgInvitations
          :items="org.invitations || []"
          @cancel-invitation="handleCancelInvitation"
          @resend-invitation="handleResendInvitation"
          @refresh-table="refreshOrg"
        />
      </div>
    </div>
  </UCard>

  <UCard v-else-if="status === 'pending'">
    <div class="flex items-center justify-center p-8">
      <UIcon
        name="i-lucide-loader-2"
        class="size-6 animate-spin text-muted"
      />
    </div>
  </UCard>

  <UCard v-else>
    <div class="flex flex-col items-center justify-center p-8 gap-4">
      <UIcon
        name="i-lucide-building-2"
        class="size-12 text-muted"
      />
      <p class="text-muted">
        Organization not found or you don't have access.
      </p>
      <UButton
        to="/dash/orgs"
        variant="outline"
        label="Back to Organizations"
      />
    </div>
  </UCard>
</template>
