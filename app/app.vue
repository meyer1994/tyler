<script setup lang="ts">
useHead({
  title: 'flower',
  titleTemplate: '%s',
  htmlAttrs: {
    lang: 'en',
    dir: 'ltr',
  },
  meta: [
    {
      name: 'description',
      content: 'Nuxt 4 + Nuxt UI 4 + tRPC + Drizzle ORM + Better Auth + Cloudflare Workers',
    },
  ],
})
</script>

<template>
  <UApp>
    <NuxtRouteAnnouncer />
    <NuxtLoadingIndicator />
    <NuxtPage />
  </UApp>
</template>
