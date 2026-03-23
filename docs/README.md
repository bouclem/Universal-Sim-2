# Universal Sim 2

A 3D universal simulation game — sequel to [Universal Sim](https://github.com/bouclem/UNIVERSAL-SIM).

Built with C# (.NET 6), Silk.NET, and Vulkan.

## Requirements

- .NET 6 SDK
- Vulkan-capable GPU + drivers
- Vulkan SDK (for shader compilation with `glslc`)

## Run

```bash
dotnet run
```

## Controls

- ENTER — Play
- Q — Quit (from menu)
- WASD — Move
- Mouse — Look around
- Space / Shift — Up / Down
- ESC — Unlock cursor / Back to menu

## Project Structure

```
Program.cs                          — Entry point
src/Game.cs                         — Window + game loop
src/MainMenu.cs                     — Menu state
src/GameScene.cs                    — 3D scene
src/objects/                        — Game objects
src/rendering/VulkanEngine.cs       — Vulkan renderer
src/rendering/Camera.cs             — Free camera
src/rendering/MeshGenerator.cs      — Sphere mesh generation
src/rendering/ShaderHelper.cs       — SPIR-V shader loading
src/shaders/                        — GLSL + compiled SPIR-V
src/assets/                         — Icons, images
docs/                               — Documentation
```
