namespace UniversalSim2.Rendering;

public static class ShaderHelper
{
    private static readonly string ShaderDir = Path.Combine(
        AppDomain.CurrentDomain.BaseDirectory, "shaders");

    public static byte[] GetVertexShaderSpirV()
        => File.ReadAllBytes(Path.Combine(ShaderDir, "vert.spv"));

    public static byte[] GetFragmentShaderSpirV()
        => File.ReadAllBytes(Path.Combine(ShaderDir, "frag.spv"));
}
