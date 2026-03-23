# Changelog — Universal Sim 2

## v0.0.2 — 2026-03-23
- Switched rendering from Raylib to Silk.NET + Vulkan
- Real 3D sphere mesh (32x32 UV sphere) with vertex normals
- Vulkan pipeline: instance, device, swapchain, render pass, graphics pipeline
- GLSL shaders with directional lighting (compiled to SPIR-V)
- Fullscreen window
- Free camera with WASD + mouse movement
- Keyboard-based menu (ENTER=Play, Q=Quit, ESC=back)

## v0.0.1 — 2026-03-23
- Initial project setup (C# / .NET 6 / Raylib-cs)
- Main menu with PLAY and QUIT buttons
- Game scene: black screen with a yellow star prototype
- Project structure: separated into Game, MainMenu, GameScene, UI, and Objects
