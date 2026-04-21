# Changelog

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
