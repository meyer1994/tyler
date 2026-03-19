<script setup lang="ts">
import type { AuthFormField, ButtonProps } from '@nuxt/ui'
import * as z from 'zod'

const schema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
  remember: z.boolean().optional().default(false),
})

const toast = useToast()
const { client: auth, loggedIn } = useAuth()

const fields: AuthFormField[] = [
  {
    name: 'email',
    label: 'Email',
    type: 'email',
    placeholder: 'Enter your email',
    required: true,
  },
  {
    name: 'password',
    label: 'Password',
    type: 'password',
    placeholder: 'Enter your password',
    required: true,
  },
  {
    name: 'remember',
    label: 'Remember me',
    type: 'checkbox',
    required: false,
  },
]

const providers = computed<ButtonProps[]>(() => [
  {
    label: 'Google',
    icon: 'i-simple-icons-google',
    class: 'p-2',
    onClick: async () => { toast.add({ title: 'Google', description: 'Sign in with Google' }) },
  },
  {
    label: 'GitHub',
    icon: 'i-simple-icons-github',
    class: 'p-2',
    onClick: async () => { toast.add({ title: 'GitHub', description: 'Sign in with GitHub' }) },
  },
  {
    label: 'Magic Link',
    icon: 'i-lucide-sparkles',
    class: 'p-2',
    onClick: async () => { toast.add({ title: 'Magic Link', description: 'Check your email for a magic link' }) },
  },
])
</script>

<template>
  <div class="flex flex-col gap-4">
    <NavHeader />

    <UContainer class="w-full md:w-md">
      <UPageCard v-if="!loggedIn">
        <AsyncSubmit
          v-slot="{ submit, status }"
          :schema="schema"
          :handler="async (data) => await auth.signIn.email({
            email: data.email,
            password: data.password,
            callbackURL: '/dash',
          })"
        >
          <UAuthForm
            :schema="schema"
            title="Sign in"
            description="Enter your credentials to access your account."
            icon="i-lucide-user"
            :fields="fields"
            :providers="providers"
            :loading="status === 'pending'"
            @submit="submit($event.data)"
          />
        </AsyncSubmit>
      </UPageCard>
    </UContainer>
  </div>
</template>
