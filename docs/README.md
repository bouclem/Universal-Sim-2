# Universal Sim 2

A 3D procedural space simulation combining ideas from Universe Sandbox² and SpaceEngine.

## Version 0.3.0

Procedural space simulation with:
- N-body gravitational physics (Velocity Verlet)
- Procedurally generated star with name, mass, granulation, sunspots
- 4-8 planets with procedural names, masses, and improved surface detail
- Moons, planetary rings, and atmospheres
- HUD overlay: selected body info, simulation speed, controls
- Orbit trail visualization
- LOD icosphere rendering for all celestial bodies
- Domain-warped noise, ridged mountains, Voronoi craters (no textures)
- Free-fly camera with crosshair selection

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
- P - Pause / Resume simulation
- +/- - Speed up / slow down simulation
- H - Toggle HUD overlay
- T - Toggle orbit trails
- F11 - Toggle fullscreen
- Escape - Quit

## Dependencies

All fetched automatically via CMake FetchContent:
- GLFW 3.4
- GLM 1.0.1
- GLAD 2.0.8 (OpenGL 4.1 core)
