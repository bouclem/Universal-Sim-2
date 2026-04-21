# Changelog

## v0.2.0 - 2026-04-21
- Orbital mechanics: planets orbit the star with Keplerian ellipses (eccentricity + inclination)
- Moons: procedurally generated (0-5 per planet), orbit their parent planet
- Planetary rings: procedural annulus with gaps, bands, and planet shadow (gas/ice giants)
- Atmosphere rendering: Fresnel rim glow with Rayleigh-like scattering approximation
- Time controls: P to pause/resume, +/- to adjust simulation speed
- Console output now lists planet details (type, moons, rings, atmosphere)

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
