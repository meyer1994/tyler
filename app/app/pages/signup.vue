<script setup lang="ts">
import type { AuthFormField, FormSubmitEvent } from '@nuxt/ui'
import * as z from 'zod'

const schema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
})

type Schema = z.output<typeof schema>

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
]

const onSubmit = async (e: FormSubmitEvent<Schema>) => {
  await auth.signUp.email({
    name: e.data.email,
    email: e.data.email,
    password: e.data.password,
    callbackURL: '/dash',
  })
  await navigateTo('/dash')
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <NavHeader />

    <UContainer class="w-full md:w-md">
      <UPageCard v-if="!loggedIn">
        <UAuthForm
          :schema="schema"
          title="Sign up"
          description="Create an account to get started."
          icon="i-lucide-user-plus"
          :fields="fields"
          @submit="async (e: FormSubmitEvent<unknown>) => await onSubmit(e as FormSubmitEvent<Schema>)"
        />
      </UPageCard>
    </UContainer>
  </div>
</template>
