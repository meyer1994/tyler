// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  modules: [
    '@nuxt/eslint',
    '@vueuse/nuxt',
    '@nuxt/ui',
    'nitro-cloudflare-dev',
    'nuxt-maplibre',
  ],

  devtools: {
    enabled: false,
    timeline: {
      enabled: true,
    },
  },

  css: ['~/assets/css/main.css'],

  runtimeConfig: {
    database: {
      url: process.env.DATABASE_URL,
    },
  },

  build: {
    transpile: ['trpc-nuxt', 'leaflet'],
  },

  compatibilityDate: '2026-03-16',

  nitro: {
    preset: 'cloudflare_module',

    cloudflare: {
      deployConfig: true,
      nodeCompat: true,
    },
  },

  vite: {
    optimizeDeps: {
      include: [
        'leaflet',
        'maplibre-gl',
        '@trpc/client',
        'better-auth/vue',
      ],
    },
  },

  typescript: {
    typeCheck: false,
    strict: true,
  },

  eslint: {
    checker: true,
    config: { stylistic: true },
  },
})
