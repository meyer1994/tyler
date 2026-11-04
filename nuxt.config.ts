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
    // tiles api server
    tiles: process.env.NUXT_TILES,
    // postgres database
    database: {
      url: process.env.DATABASE_URL,
    },
  },

  build: {
    transpile: ['trpc-nuxt', 'leaflet'],
  },

  compatibilityDate: '2026-08-24',

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
        'justice-steady-woods-sets.trycloudflare.com',
      ],
    },
  },

  typescript: {
    typeCheck: false,
    strict: true,
  },

  echarts: {
    renderer: 'canvas',
    charts: ['LineChart'],
    components: ['GridComponent', 'TitleComponent', 'TooltipComponent'],
  },

  eslint: {
    checker: true,
    config: { stylistic: true },
  },
})
