using System.Numerics;
using Raylib_cs;

namespace UniversalSim2;

public enum GameState
{
    MainMenu,
    Playing
}

public static class Game
{
    public const int ScreenWidth = 1280;
    public const int ScreenHeight = 720;
    public const string Title = "Universal Sim 2";

    public static GameState State = GameState.MainMenu;

    public static void Run()
    {
        Raylib.InitWindow(ScreenWidth, ScreenHeight, Title);
        Raylib.SetTargetFPS(60);

        Camera3D camera = new Camera3D
        {
            Position = new Vector3(0.0f, 2.0f, 8.0f),
            Target = new Vector3(0.0f, 0.0f, 0.0f),
            Up = new Vector3(0.0f, 1.0f, 0.0f),
            FovY = 45.0f,
            Projection = CameraProjection.Perspective
        };

        while (!Raylib.WindowShouldClose())
        {
            // --- UPDATE ---
            switch (State)
            {
                case GameState.MainMenu:
                    MainMenu.Update();
                    break;
                case GameState.Playing:
                    break;
            }

            // --- DRAW ---
            Raylib.BeginDrawing();

            switch (State)
            {
                case GameState.MainMenu:
                    MainMenu.Draw();
                    break;
                case GameState.Playing:
                    GameScene.Draw(camera);
                    break;
            }

            Raylib.EndDrawing();
        }

        Raylib.CloseWindow();
    }
}
