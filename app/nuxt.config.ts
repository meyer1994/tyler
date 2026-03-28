// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  modules: [
    '@nuxt/eslint',
    '@vueuse/nuxt',
    '@nuxt/ui',
    'nuxt-maplibre',
    'nuxt-echarts',
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

  compatibilityDate: '2026-03-28',

  nitro: {
    preset: 'node-server',
  },

  vite: {
    optimizeDeps: {
      include: [
        '@indoorequal/vue-maplibre-gl',
        '@mapbox/mapbox-gl-draw',
        '@trpc/client',
        '@vue-leaflet/vue-leaflet',
        'better-auth/vue',
        'leaflet',
        'maplibre-gl',
        'zod',
      ],
    },
    server: {
      allowedHosts: [
        'free-loudly-dingo.ngrok-free.app',
        'himself-spy-keyboards-foster.trycloudflare.com',
      ],
    },
  },

  typescript: {
    typeCheck: false,
    strict: true,
  },

  echarts: {
    charts: ['LineChart'],
    components: ['GridComponent', 'TitleComponent', 'TooltipComponent'],
  },

  eslint: {
    checker: true,
    config: { stylistic: true },
  },
})
