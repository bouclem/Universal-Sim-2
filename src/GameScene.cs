using System.Numerics;
using Raylib_cs;
using UniversalSim2.Objects;

namespace UniversalSim2;

public static class GameScene
{
    private static readonly YellowStarPrototype Star = new(Vector3.Zero, 1.0f);

    public static void Draw(Camera3D camera)
    {
        Raylib.ClearBackground(Color.Black);

        Raylib.BeginMode3D(camera);

        Star.Draw();

        Raylib.EndMode3D();
    }
}
