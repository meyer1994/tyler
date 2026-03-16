<script setup lang="ts">
import type { AuthFormField, ButtonProps, FormSubmitEvent } from '@nuxt/ui'
import * as z from 'zod'

const schema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
  remember: z.boolean().optional().default(false),
})

type Schema = z.output<typeof schema>

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

const form = useTemplateRef('form')

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
    onClick: async () => {
      const email = z.email().safeParse(form.value?.state.email)

      if (!email.success) {
        toast.add({
          color: 'error',
          title: 'Invalid Email',
          description: 'Please enter a valid email address',
        })
        return
      }

      await auth.signIn.magicLink({ email: email.data, callbackURL: '/dash' })
      toast.add({
        title: 'Magic Link',
        description: 'Check your email for a magic link',
      })
    },
  },
])

const onSubmit = async (e: FormSubmitEvent<Schema>) => {
  await auth.signIn.email({
    email: e.data.email,
    password: e.data.password,
    callbackURL: '/dash',
  })
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <NavHeader />

    <UContainer class="w-full md:w-md">
      <UPageCard v-if="!loggedIn">
        <UAuthForm
          ref="form"
          :schema="schema"
          title="Sign in"
          description="Enter your credentials to access your account."
          icon="i-lucide-user"
          :fields="fields"
          :providers="providers"
          @submit="async (e: FormSubmitEvent<unknown>) => await onSubmit(e as FormSubmitEvent<Schema>)"
        />
      </UPageCard>
    </UContainer>
  </div>
</template>
