<script setup lang="ts">
import type { AuthFormField, FormSubmitEvent } from '@nuxt/ui'
import * as z from 'zod'

const mode = ref<'signin' | 'signup'>('signin')
const loading = ref(false)

const signinSchema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
})

const signupSchema = z.object({
  email: z.email('Invalid email'),
  password: z.string('Password is required').min(8, 'Must be at least 8 characters'),
})

type SigninSchema = z.output<typeof signinSchema>
type SignupSchema = z.output<typeof signupSchema>

const toast = useToast()
const { $auth } = useNuxtApp()
const { data: session } = $auth.useSession()
const loggedIn = computed(() => !!session.value?.user)

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

const onSignin = async (event: FormSubmitEvent<SigninSchema>) => {
  loading.value = true
  const { error, data } = await $auth.signIn.email({
    email: event.data.email,
    password: event.data.password,
    callbackURL: '/dash',
  })
  loading.value = false

  if (error) {
    console.error(error)
    toast.add({ title: 'Sign-in failed', description: error.message, color: 'error' })
    return
  }

  if (data) await reloadNuxtApp({ path: '/dash' })
}

const onSignup = async (event: FormSubmitEvent<SignupSchema>) => {
  loading.value = true
  const { error, data } = await $auth.signUp.email({
    name: event.data.email,
    email: event.data.email,
    password: event.data.password,
    callbackURL: '/dash',
  })
  loading.value = false

  if (error) {
    console.error(error)
    toast.add({ title: 'Sign-up failed', description: error.message, color: 'error' })
    return
  }

  if (data) await reloadNuxtApp({ path: '/dash' })
}
</script>

<template>
  <div class="flex flex-col lg:flex-row min-h-screen">
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

    <div class="flex-1 flex items-center justify-center p-6">
      <div
        v-if="!loggedIn"
        class="w-full max-w-md"
      >
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

        <UAuthForm
          v-if="mode === 'signin'"
          :schema="signinSchema"
          title="Welcome Back"
          description="Enter your credentials to access."
          :fields="signinFields"
          :loading="loading"
          @submit="onSignin"
        />

        <UAuthForm
          v-else
          :schema="signupSchema"
          title="Create Account"
          description="Sign up to start tracking your devices."
          :fields="signupFields"
          :loading="loading"
          @submit="onSignup"
        />
      </div>
    </div>
  </div>
</template>
