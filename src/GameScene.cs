using System.Numerics;
using UniversalSim2.Objects;

namespace UniversalSim2;

public static class GameScene
{
    private static YellowStarPrototype _star = null!;

    public static void Initialize()
    {
        _star = new YellowStarPrototype(Vector3.Zero, 1.0f);
    }

    public static float[] GetVertices() => _star.Vertices;
    public static uint[] GetIndices() => _star.Indices;
}
