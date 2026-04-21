# Universal Sim 2

A 3D procedural space simulation combining ideas from Universe Sandbox² and SpaceEngine.

## Version 0.4.0

Procedural space simulation with:
- N-body gravitational physics (Velocity Verlet)
- Procedurally generated star with animated granulation, sunspots, corona
- 4-8 planets with realistic procedural surfaces (biomes, oceans, storms, haze)
- Moons, planetary rings, and atmospheres with Rayleigh/Mie scattering
- Asteroid belt with 2000 instanced bodies
- Orbit path prediction lines (full Keplerian ellipse)
- Body focus/follow camera mode
- Collision detection and response (momentum-conserving merge)
- HUD overlay: selected body info, simulation speed, controls
- Orbit trail visualization
- Milky Way background with nebulae and multi-layer starfield
- LOD icosphere rendering for all celestial bodies
- Free-fly camera with crosshair selection

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
- R - Regenerate solar system
- P - Pause / Resume simulation
- +/- - Speed up / slow down simulation
- H - Toggle HUD overlay
- T - Toggle orbit trails
- O - Toggle orbit prediction lines
- F - Follow selected body
- F11 - Toggle fullscreen
- Escape - Exit follow mode / Quit

## Dependencies

All fetched automatically via CMake FetchContent:
- GLFW 3.4
- GLM 1.0.1
- GLAD 2.0.8 (OpenGL 4.1 core)
