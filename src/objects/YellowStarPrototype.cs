using System.Numerics;
using Raylib_cs;

namespace UniversalSim2.Objects;

public class YellowStarPrototype
{
    public Vector3 Position { get; set; }
    public float Radius { get; set; }

    public YellowStarPrototype(Vector3 position, float radius)
    {
        Position = position;
        Radius = radius;
    }

    public void Draw()
    {
        // Solid yellow sphere
        Raylib.DrawSphere(Position, Radius, Color.Yellow);

        // Glow rings to simulate light emission
        Raylib.DrawCircle3D(
            Position, Radius * 1.4f,
            new Vector3(1.0f, 0.0f, 0.0f), 90.0f,
            new Color(255, 255, 100, 80));
        Raylib.DrawCircle3D(
            Position, Radius * 1.4f,
            new Vector3(0.0f, 0.0f, 1.0f), 90.0f,
            new Color(255, 255, 100, 80));
        Raylib.DrawCircle3D(
            Position, Radius * 1.8f,
            new Vector3(1.0f, 0.0f, 0.0f), 90.0f,
            new Color(255, 200, 50, 40));
        Raylib.DrawCircle3D(
            Position, Radius * 1.8f,
            new Vector3(0.0f, 0.0f, 1.0f), 90.0f,
            new Color(255, 200, 50, 40));
    }
}
