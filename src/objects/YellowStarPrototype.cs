using System.Numerics;
using UniversalSim2.Rendering;

namespace UniversalSim2.Objects;

public class YellowStarPrototype
{
    public Vector3 Position { get; set; }
    public float Radius { get; set; }

    public float[] Vertices { get; }
    public uint[] Indices { get; }

    public YellowStarPrototype(Vector3 position, float radius)
    {
        Position = position;
        Radius = radius;

        // Generate a real 3D sphere mesh (32 stacks x 32 slices)
        (Vertices, Indices) = MeshGenerator.CreateSphere(radius, 32, 32);
    }
}
