export enum View {
  /**
   * RGB
   */
  RGB = 'rgb',

  /**
   * NDVI = (NIR - RED) / (NIR + RED)
   */
  NDVI = 'ndvi',
}

export enum Collections {
  // not work (without paying, probably)
  // COP_DEM_GLO_30 = 'cop-dem-glo-30',
  // COP_DEM_GLO_90 = 'cop-dem-glo-90',
  // LANDSAT_C2_L2 = 'landsat-c2-l2',
  // NAIP = 'naip',
  // SENTINEL_1_GRD = 'sentinel-1-grd',
  // work (free)
  SENTINEL_2_C1_L2A = 'sentinel-2-c1-l2a',
  SENTINEL_2_L2A = 'sentinel-2-l2a',
}
