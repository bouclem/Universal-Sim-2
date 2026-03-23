using System.Numerics;
using Raylib_cs;

namespace UniversalSim2;

public static class UI
{
    public static void DrawButton(string text, int x, int y, int width, int height, bool hovered)
    {
        Color bg = hovered ? new Color(60, 60, 100, 255) : new Color(30, 30, 60, 255);
        Color border = hovered ? Color.White : Color.Gray;

        Raylib.DrawRectangle(x, y, width, height, bg);
        Raylib.DrawRectangleLines(x, y, width, height, border);

        int textWidth = Raylib.MeasureText(text, 24);
        int textX = x + width / 2 - textWidth / 2;
        int textY = y + height / 2 - 12;
        Raylib.DrawText(text, textX, textY, 24, Color.White);
    }

    public static bool IsInsideRect(Vector2 point, int x, int y, int w, int h)
    {
        return point.X >= x && point.X <= x + w &&
               point.Y >= y && point.Y <= y + h;
    }
}
