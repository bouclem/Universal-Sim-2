# Changelog

## v0.6.0 - 2026-04-21
- Realistic size ratios: stars ~100x rocky planets, ~10x gas giants, wider orbital spacing
- LOD system upgraded from 6 to 8 levels (LOD 7 = 327K triangles for close-up detail)
- Planetary rotation and axial tilt with visible surface spin
- Massively upgraded procedural textures:
  - Rocky planets: tectonic plate boundaries with rift valleys, hydraulic erosion noise, volcanic hotspots with lava glow (night-side visible), river-like erosion channels, expanded biome system (desert, savanna, grassland, forest, rainforest, wetland, steppe, tundra, boreal), layered rock strata, patchy wind-exposed snow, continental shelf ocean coloring, wave patterns, multi-scale impact craters, sand dune patterns, smarter city light placement near coasts and temperate zones
  - Gas giants: 4-tier banding (added ultra-fine bands), jet streams at band boundaries, ammonia crystal cloud tops, 5 storm vortices with varied sizes, multi-layer Great Spot swirling with bright ring, lightning flash spots in deep storms, finer atmospheric grain
  - Ice giants: wind shear patterns at different altitudes, multiple methane haze layers, cirrus-like streaks, secondary dark spot, aurora bands at poles, more micro-detail
  - Stars: 3-scale granulation (supergranulation + primary + fine), sunspot penumbra radial filaments, magnetic field-aligned bright loops, solar prominences at the limb, structured corona with streamers, stronger wavelength-dependent limb darkening
- Multi-scale bump mapping with tectonic plate boundary detail
- Subtle cloud-top bump mapping for gas giants
- Erosion noise function simulating hydraulic erosion
- Crackle noise (Voronoi F2-F1) for tectonic plate boundaries
- Triple domain warping for extreme organic terrain distortion
- Gravitational constant retuned for new mass/distance scales
- Wider system enter/exit thresholds for larger solar systems
- Generous selection hit radius for small bodies

## v0.5.0 - 2026-04-21
- Multi-scale universe: seamless zoom from galaxy → star field → solar system
- Procedural spiral galaxy with 3000 stars across 4 spiral arms + core bulge
- Galaxy view: dust cloud rendering with spiral arm structure, emission nebulae, dust lanes
- Star field view: individual stars as glowing point sprites with distance-based sizing
- Solar systems generated on-demand when approaching a star
- Binary star systems: ~15% of stars have an orbiting companion
- Camera speed auto-scales with viewing distance (galaxy/star field/solar system)
- HUD shows current viewing scale, nearest star info in star field view
- R key regenerates the entire galaxy
- Escape exits follow mode before quitting

## v0.4.0 - 2026-04-21
- Orbit path prediction lines (full Keplerian ellipse from current state, toggle with O)
- Body focus/follow camera mode (F to lock onto selected body, Escape to exit)
- Collision detection and response (momentum-conserving merge, volume addition)
- Asteroid belt: 2000 instanced asteroids placed in the largest orbital gap
- Massively upgraded procedural textures:
  - Rocky planets: continents/oceans with depth coloring, biome system (desert, grassland, forest, tundra, snow), tectonic ridges (Swiss turbulence), bump mapping from noise derivatives, ocean Fresnel specular, faint night-side city lights, subsurface scattering at terminator, tone mapping
  - Gas giants: 3-tier banding with domain-warped distortion, chevron patterns, Great Spot with swirling interior and dark eye, smaller storm vortices, belt turbulence
  - Ice giants: methane-blue haze layers, wispy cloud streaks, dark spot with bright companion, polar aurora glow
  - Star: animated Voronoi granulation, sunspot umbra/penumbra, faculae, wavelength-dependent limb darkening, corona glow, HDR bloom
  - Background: Milky Way band with dust lanes, colored nebula patches, 3-layer starfield with diffraction spikes and glow halos, star cluster glow
  - Atmosphere: Rayleigh phase function, Mie forward scattering, sunset/sunrise coloring, atmospheric extinction
  - Rings: 4-layer density + particle noise, 4 named gaps, inner-to-outer color gradient, forward scattering translucency, soft penumbra shadows
- Gradient noise (Perlin-style) replaces value noise for smoother, less pixelated surfaces
- Escape key exits follow mode before quitting

## v0.3.0 - 2026-04-21
- N-body gravitational simulation (Velocity Verlet integration, replaces fixed Kepler orbits)
- Bodies have mass, velocity, and gravitational interactions
- Procedural name generator for star and planets (syllable-based)
- HUD overlay with selected body info (name, type, radius, mass, distance, velocity)
- Crosshair selection: look at a body to see its info
- Orbit trail visualization (fading line strips, toggle with T)
- Toggle HUD with H
- Much improved procedural textures:
  - Rocky planets: domain warping, ridged noise mountains, Voronoi craters, noisy ice caps
  - Gas giants: multi-layer banding, domain-warped storms, Great Spot feature
  - Ice giants: wispy cloud layers, polar brightening
  - Star: granulation cells (Voronoi), sunspots, solar flares, quadratic limb darkening
- Improved lighting: soft terminator, rim backlight, quintic noise interpolation
- R key now properly toggles (no repeat on hold)

## v0.2.0 - 2026-04-21
- Orbital mechanics: planets orbit the star with Keplerian ellipses (eccentricity + inclination)
- Moons: procedurally generated (0-5 per planet), orbit their parent planet
- Planetary rings: procedural annulus with gaps, bands, and planet shadow (gas/ice giants)
- Atmosphere rendering: Fresnel rim glow with Rayleigh-like scattering approximation
- Time controls: P to pause/resume, +/- to adjust simulation speed
- F11 fullscreen toggle
- Application icon embedded in executable

## v0.1.0 - 2026-04-21
- Initial release
- Procedural solar system generation (star + 4-8 planets)
- Star with blackbody color from random temperature
- Three planet types: rocky, gas giant, ice giant
- LOD icosphere rendering (6 levels, distance-based for all bodies)
- Procedural surface coloring via simplex noise (no textures)
- Background procedural starfield
- Free-fly camera with WASD + mouse
- Regenerate system with R key
