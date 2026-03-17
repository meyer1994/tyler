<script setup lang="ts">
const { loggedIn } = useAuth()

const heroLinks = computed(() => {
  if (loggedIn.value) {
    return [
      { label: 'Go to Dashboard', to: '/dash', icon: 'i-lucide-layout-dashboard', color: 'primary' as const },
    ]
  }
  return [
    { label: 'Get Started', to: '/signup', icon: 'i-lucide-rocket', color: 'primary' as const },
    { label: 'Sign In', to: '/signin', icon: 'i-lucide-log-in', color: 'neutral' as const },
  ]
})

const features = [
  {
    title: 'Database',
    description: 'Drizzle ORM with SQLite/D1 for type-safe database access.',
    icon: 'i-lucide-database',
  },
  {
    title: 'File Storage',
    description: 'S3-compatible storage with Cloudflare R2 integration.',
    icon: 'i-lucide-hard-drive',
  },
  {
    title: 'Vector Search',
    description: 'Semantic search powered by Cloudflare Vectorize and Workers AI.',
    icon: 'i-lucide-brain-circuit',
  },
  {
    title: 'Authentication',
    description: 'Secure auth with Better Auth and multiple providers.',
    icon: 'i-lucide-shield-check',
  },
  {
    title: 'Type-Safe API',
    description: 'End-to-end type safety with tRPC between client and server.',
    icon: 'i-lucide-code',
  },
  {
    title: 'Modern UI',
    description: 'Beautiful components with Nuxt UI and Tailwind CSS.',
    icon: 'i-lucide-palette',
  },
]
</script>

<template>
  <div class="flex flex-col">
    <NavHeader />

    <UPageHero
      title="Nuxt Template"
      description="A production-ready Nuxt 4 template with authentication, database, file storage, and vector search built-in."
      :links="heroLinks"
    >
      <template #headline>
        <UBadge
          color="primary"
          variant="subtle"
        >
          Full-Stack Ready
        </UBadge>
      </template>
    </UPageHero>

    <UPageSection
      title="Everything You Need"
      description="Start building your application with a complete set of features out of the box."
      :features="features"
    />

    <UPageSection>
      <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
        <UPageCard
          title="Quick Start"
          description="Clone the repo, install dependencies, and start building in minutes."
        >
          <div class="flex flex-col gap-2">
            <code class="text-sm text-muted">pnpm install</code>
            <code class="text-sm text-muted">pnpm dev</code>
          </div>
        </UPageCard>

        <UPageCard
          title="Deploy Anywhere"
          description="Ready for deployment to Cloudflare Workers, Vercel, or any Node.js host."
        >
          <div class="flex gap-2">
            <UIcon
              name="i-lucide-cloud"
              class="size-5 text-muted"
            />
            <UIcon
              name="i-lucide-server"
              class="size-5 text-muted"
            />
            <UIcon
              name="i-lucide-globe"
              class="size-5 text-muted"
            />
          </div>
        </UPageCard>
      </div>
    </UPageSection>
  </div>
</template>
