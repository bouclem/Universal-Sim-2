# Universal Sim 2

A 3D universal simulation game — sequel to [Universal Sim](https://github.com/bouclem/UNIVERSAL-SIM).

Built with C# (.NET 6) and [Raylib-cs](https://github.com/raylib-cs/raylib-cs).

## Requirements

- .NET 6 SDK

## Run

```bash
dotnet run
```

## Controls

- Main menu: click PLAY to start, QUIT to exit.
- ESC closes the window at any time.

## Project Structure

```
Program.cs              — Entry point
src/Game.cs             — Main game loop
src/MainMenu.cs         — Menu screen
src/GameScene.cs        — 3D game scene
src/UI.cs               — Shared UI helpers
src/objects/             — Game objects
src/assets/             — Icons, images
docs/                   — Documentation
```
