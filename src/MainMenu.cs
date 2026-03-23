using System.Numerics;
using Raylib_cs;

namespace UniversalSim2;

public static class MainMenu
{
    private const int ButtonWidth = 200;
    private const int ButtonHeight = 50;

    public static void Update()
    {
        if (!Raylib.IsMouseButtonPressed(MouseButton.Left)) return;

        Vector2 mouse = Raylib.GetMousePosition();

        int playX = Game.ScreenWidth / 2 - ButtonWidth / 2;
        int playY = Game.ScreenHeight / 2 - 10 - ButtonHeight;
        if (UI.IsInsideRect(mouse, playX, playY, ButtonWidth, ButtonHeight))
        {
            Game.State = GameState.Playing;
        }

        int quitX = Game.ScreenWidth / 2 - ButtonWidth / 2;
        int quitY = Game.ScreenHeight / 2 + 10;
        if (UI.IsInsideRect(mouse, quitX, quitY, ButtonWidth, ButtonHeight))
        {
            Raylib.CloseWindow();
            Environment.Exit(0);
        }
    }

    public static void Draw()
    {
        Raylib.ClearBackground(new Color(10, 10, 30, 255));

        // Title
        const string titleText = "UNIVERSAL SIM 2";
        int titleWidth = Raylib.MeasureText(titleText, 48);
        Raylib.DrawText(titleText, Game.ScreenWidth / 2 - titleWidth / 2, Game.ScreenHeight / 4, 48, Color.White);

        // Version
        const string versionText = "v0.0.1";
        int versionWidth = Raylib.MeasureText(versionText, 20);
        Raylib.DrawText(versionText, Game.ScreenWidth / 2 - versionWidth / 2, Game.ScreenHeight / 4 + 58, 20, Color.Gray);

        Vector2 mouse = Raylib.GetMousePosition();

        // PLAY button
        int playX = Game.ScreenWidth / 2 - ButtonWidth / 2;
        int playY = Game.ScreenHeight / 2 - 10 - ButtonHeight;
        bool playHover = UI.IsInsideRect(mouse, playX, playY, ButtonWidth, ButtonHeight);
        UI.DrawButton("PLAY", playX, playY, ButtonWidth, ButtonHeight, playHover);

        // QUIT button
        int quitX = Game.ScreenWidth / 2 - ButtonWidth / 2;
        int quitY = Game.ScreenHeight / 2 + 10;
        bool quitHover = UI.IsInsideRect(mouse, quitX, quitY, ButtonWidth, ButtonHeight);
        UI.DrawButton("QUIT", quitX, quitY, ButtonWidth, ButtonHeight, quitHover);
    }
}
