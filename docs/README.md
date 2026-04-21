# Universal Sim 2

A 3D procedural space simulation combining ideas from Universe Sandbox² and SpaceEngine.

## Version 0.2.0

Procedural solar system with:
- Procedurally generated star (blackbody color from temperature)
- 4-8 procedurally generated planets (rocky, gas giant, ice giant)
- Procedurally generated moons (0-5 per planet)
- Planetary rings on gas/ice giants with procedural bands and gaps
- Atmosphere rendering with Fresnel rim glow and scattering
- Orbital mechanics: Keplerian ellipses with eccentricity and inclination
- LOD icosphere rendering for all celestial bodies
- Procedural surface coloring via noise (no textures)
- Background starfield
- Free-fly camera (WASD + mouse)

## Build

Requires CMake 3.20+ and a C++17 compiler.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Controls

- WASD - Move
- Mouse - Look around
- Space / Ctrl - Up / Down
- Shift - Move faster
- Scroll - Adjust speed
- R - Regenerate solar system
- P - Pause / Resume orbits
- +/- - Speed up / slow down simulation
- Escape - Quit

## Dependencies

All fetched automatically via CMake FetchContent:
- GLFW 3.4
- GLM 1.0.1
- GLAD 2.0.8 (OpenGL 4.1 core)
