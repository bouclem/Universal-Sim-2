# Universal Sim 2

A 3D procedural space simulation combining ideas from Universe Sandbox² and SpaceEngine.

## Version 0.6.0

Multi-scale procedural universe with seamless zoom:
- **Galaxy view** — procedural spiral galaxy with 3000 stars, dust clouds, nebulae
- **Star field view** — individual stars as glowing points, fly between systems
- **Solar system view** — full detail: planets, moons, rings, atmospheres, asteroids
- N-body gravitational physics (Velocity Verlet)
- Binary star systems (~15% of stars)
- Realistic size ratios: star ~100x rocky planets, ~10x gas giants
- Planetary rotation with axial tilt (visible surface spin)
- 8-level LOD icosphere (up to 327K triangles for close-up detail)
- Procedurally generated stars with multi-scale granulation, sunspots, prominences, corona
- Rocky planets: tectonic plates, erosion, volcanoes, lava glow, 10+ biomes, rivers, craters
- Gas giants: 4-tier banding, jet streams, ammonia clouds, lightning, Great Spot
- Ice giants: wind shear, methane haze layers, dark spots, aurora bands
- Moons, planetary rings, and atmospheres with Rayleigh/Mie scattering
- Asteroid belts with instanced rendering
- Orbit path prediction and trail visualization
- Body focus/follow camera mode
- Collision detection and response
- Milky Way background with nebulae and multi-layer starfield
- Camera speed auto-scales with viewing distance

## Build

Requires CMake 3.20+ and a C++17 compiler.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Controls

- WASD - Move (orbit in follow mode)
- Mouse - Look around (orbit in follow mode)
- Space / Ctrl - Up / Down (zoom in follow mode)
- Shift - Move faster
- Scroll - Adjust speed (zoom in follow mode)
- R - Regenerate galaxy
- P - Pause / Resume simulation
- +/- - Speed up / slow down simulation
- H - Toggle HUD overlay
- T - Toggle orbit trails
- O - Toggle orbit prediction lines
- F - Follow selected body
- F11 - Toggle fullscreen
- Escape - Exit follow mode / Quit

## Scale Transitions

- Fly away from a star system to see the star field
- Keep going to see the full galaxy spiral
- Approach any star to enter its solar system (generated on demand)
- Speed automatically adjusts to the current scale

## Dependencies

All fetched automatically via CMake FetchContent:
- GLFW 3.4
- GLM 1.0.1
- GLAD 2.0.8 (OpenGL 4.1 core)
