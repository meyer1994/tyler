<script setup lang="ts">
import type { AuthFormField, ButtonProps, FormSubmitEvent } from '@nuxt/ui'
import * as z from 'zod'

const mode = ref<'signin' | 'signup'>('signin')

const signinSchema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
})

const signupSchema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
})

const toast = useToast()
const { client: auth, loggedIn } = useAuth()

const signinFields: AuthFormField[] = [
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

const signupFields: AuthFormField[] = [
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
    placeholder: 'Choose a password',
    required: true,
  },
]

const providers = computed<ButtonProps[]>(() => [
  {
    label: 'GitHub',
    icon: 'i-simple-icons-github',
    class: 'p-2',
    onClick: async () => { toast.add({ title: 'GitHub', description: 'Sign in with GitHub' }) },
  },
  {
    label: 'Google',
    icon: 'i-simple-icons-google',
    class: 'p-2',
    onClick: async () => { toast.add({ title: 'Google', description: 'Sign in with Google' }) },
  },
])

type SignupSchema = z.output<typeof signupSchema>

const onSignup = async (e: FormSubmitEvent<SignupSchema>) => {
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
  <div class="flex flex-col lg:flex-row min-h-screen">
    <!-- Left branding panel -->
    <div class="hidden lg:flex lg:w-1/2 bg-gray-950 p-12 flex-col justify-between">
      <div>
        <div class="flex items-center gap-2 mb-12">
          <UIcon
            name="i-lucide-flower"
            class="text-3xl text-primary"
          />
          <span class="text-2xl font-bold text-white">trackows</span>
        </div>
        <h1 class="text-5xl font-bold text-white mb-4">
          Track Your
          <br>
          <span class="text-primary">Devices</span>
        </h1>
        <p class="text-gray-400 text-lg">
          Real-time GPS tracking and telemetry for your connected devices.
        </p>
      </div>

      <div>
        <div class="flex items-center gap-2 text-sm">
          <div class="w-2 h-2 rounded-full bg-green-500 animate-pulse" />
          <span class="text-gray-300">System Online</span>
        </div>
      </div>
    </div>

    <!-- Right form panel -->
    <div class="flex-1 flex items-center justify-center p-6">
      <div
        v-if="!loggedIn"
        class="w-full max-w-md"
      >
        <!-- Tab toggle -->
        <div class="flex gap-1 mb-8">
          <button
            class="flex-1 py-2 px-4 text-sm font-medium transition-colors rounded"
            :class="mode === 'signin' ? 'bg-primary text-white' : 'text-gray-600 dark:text-gray-400'"
            @click="mode = 'signin'"
          >
            Sign In
          </button>
          <button
            class="flex-1 py-2 px-4 text-sm font-medium transition-colors rounded"
            :class="{
              'bg-primary text-white': mode === 'signup',
              'text-gray-600 dark:text-gray-400': mode !== 'signup',
            }"
            @click="mode = 'signup'"
          >
            Sign Up
          </button>
        </div>

        <!-- Sign In form -->
        <div v-if="mode === 'signin'">
          <AsyncSubmit
            v-slot="{ submit, status }"
            :schema="signinSchema"
            :handler="async (data) => await auth.signIn.email({
              email: data.email,
              password: data.password,
              callbackURL: '/dash',
            })"
          >
            <UAuthForm
              :schema="signinSchema"
              title="Welcome Back"
              description="Enter your credentials to access."
              :fields="signinFields"
              :providers="providers"
              :loading="status === 'pending'"
              @submit="submit($event.data)"
            />
          </AsyncSubmit>
        </div>

        <!-- Sign Up form -->
        <div v-else>
          <UAuthForm
            :schema="signupSchema"
            title="Create Account"
            description="Sign up to start tracking your devices."
            :fields="signupFields"
            :providers="providers"
            @submit="async (e: FormSubmitEvent<unknown>) => await onSignup(e as FormSubmitEvent<SignupSchema>)"
          />
        </div>
      </div>
    </div>
  </div>
</template>
