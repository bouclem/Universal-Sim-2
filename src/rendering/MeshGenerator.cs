using System.Numerics;

namespace UniversalSim2.Rendering;

public static class MeshGenerator
{
    /// <summary>
    /// Generates a UV sphere with interleaved position+normal data.
    /// Returns vertices (pos.xyz + normal.xyz per vertex) and indices.
    /// </summary>
    public static (float[] vertices, uint[] indices) CreateSphere(
        float radius, int stacks, int slices)
    {
        var verts = new List<float>();
        var idxs = new List<uint>();

        for (int i = 0; i <= stacks; i++)
        {
            float phi = MathF.PI * i / stacks;
            float y = MathF.Cos(phi);
            float sinPhi = MathF.Sin(phi);

            for (int j = 0; j <= slices; j++)
            {
                float theta = 2f * MathF.PI * j / slices;
                float x = sinPhi * MathF.Cos(theta);
                float z = sinPhi * MathF.Sin(theta);

                // Position
                verts.Add(x * radius);
                verts.Add(y * radius);
                verts.Add(z * radius);
                // Normal (unit sphere normal = position normalized)
                verts.Add(x);
                verts.Add(y);
                verts.Add(z);
            }
        }

        for (int i = 0; i < stacks; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                uint a = (uint)(i * (slices + 1) + j);
                uint b = a + (uint)(slices + 1);

                idxs.Add(a);
                idxs.Add(b);
                idxs.Add(a + 1);

                idxs.Add(a + 1);
                idxs.Add(b);
                idxs.Add(b + 1);
            }
        }

        return (verts.ToArray(), idxs.ToArray());
    }
}
