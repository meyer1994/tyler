<script setup lang="ts">
import { UseClipboard } from '@vueuse/components'
import type { ApiKey } from 'better-auth/plugins'
import type { FormApiKeyData } from '~/components/FormApiKey.vue'

const { apiKey } = useAuth()
const { data: keys, status, refresh } = await useAsyncData('dash-keys', async () => {
  const result = await apiKey.list()
  return result.data ?? []
})

const key = ref<ApiKey | null>(null)
const isCreating = ref(false)
const isShowingKey = ref(false)

async function onCreateKey(e: FormApiKeyData) {
  const result = await apiKey.create({
    name: e.name,
    prefix: e.prefix || undefined,
    expiresIn: e.expiresIn,
  })
  if (result.error) throw result.error
  key.value = result.data ?? null
  isShowingKey.value = true
  await refresh()
}

function onCloseKeyView() {
  isShowingKey.value = false
  isCreating.value = false
  key.value = null
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <UCard class="flex-1 flex flex-col">
      <template #header>
        <div class="flex items-center justify-between w-full">
          <div>
            <h1 class="text-xl font-semibold">
              API Keys
            </h1>
            <p class="text-sm text-muted mt-1">
              Manage your API keys for programmatic access
            </p>
          </div>

          <UButton
            icon="i-lucide-plus"
            label="Create Key"
            @click="isCreating = true"
          />
        </div>
      </template>

      <!-- @vue-expect-error TODO: fix this -->
      <TableApiKeys
        :items="keys ?? []"
        :loading="status === 'pending'"
        @delete-key="async (e) => {
          await apiKey.delete({ keyId: e.id })
          await refresh()
        }"
        @refresh-table="async () => {
          await refresh()
        }"
      />
    </UCard>

    <USlideover
      v-model:open="isCreating"
      title="Create API Key"
      description="Create a new API key for programmatic access"
      :ui="{ footer: 'justify-end' }"
    >
      <template #body>
        <FormApiKey
          :loading="status === 'pending'"
          @submit="async (e) => onCreateKey(e)"
        >
          <template #cancel-button>
            <UButton
              label="Cancel"
              color="neutral"
              variant="outline"
              :disabled="status === 'pending'"
              @click="isCreating = false"
            />
          </template>
          <template #submit-button>
            <UButton
              type="submit"
              label="Create Key"
              :loading="status === 'pending'"
            />
          </template>
        </FormApiKey>
      </template>

      <template #footer>
        <USlideover
          v-model:open="isShowingKey"
          title="API Key Created"
          :dismissible="false"
          :ui="{ footer: 'justify-end' }"
        >
          <template #body>
            <div class="flex flex-col gap-4">
              <UAlert
                icon="i-lucide-alert-triangle"
                color="warning"
                title="Save your key"
                description="Make sure to copy your API key now. You won't be able to see it again!"
              />

              <div class="flex flex-col gap-2">
                <label class="text-sm font-medium">Your API Key</label>
                <div class="flex gap-2">
                  <UInput
                    :model-value="key?.key"
                    readonly
                    class="flex-1 font-mono text-sm"
                  />
                  <UseClipboard
                    v-slot="{ copy, copied }"
                    :source="String(key?.key ?? '')"
                  >
                    <UButton
                      :icon="copied ? 'i-lucide-check' : 'i-lucide-copy'"
                      :color="copied ? 'success' : 'neutral'"
                      label="Copy"
                      variant="outline"
                      @click="() => copy()"
                    />
                  </UseClipboard>
                </div>
              </div>
            </div>
          </template>

          <template #footer>
            <UButton
              label="Done"
              @click="onCloseKeyView"
            />
          </template>
        </USlideover>
      </template>
    </USlideover>
  </div>
</template>
