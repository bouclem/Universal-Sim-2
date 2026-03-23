using System.Numerics;
using Silk.NET.Input;
using Silk.NET.Maths;

namespace UniversalSim2.Rendering;

public class Camera
{
    public Vector3 Position;
    public float Yaw = -90f;
    public float Pitch = 0f;
    public float Speed = 10f;
    public float Sensitivity = 0.1f;
    public float Fov = 60f;

    private Vector3 _front = -Vector3.UnitZ;
    private Vector3 _up = Vector3.UnitY;
    private Vector3 _right = Vector3.UnitX;

    private Vector2 _lastMouse;
    private bool _firstMouse = true;

    public Camera(Vector3 position)
    {
        Position = position;
        UpdateVectors();
    }

    public void ProcessKeyboard(IKeyboard keyboard, float dt)
    {
        float velocity = Speed * dt;

        if (keyboard.IsKeyPressed(Key.W)) Position += _front * velocity;
        if (keyboard.IsKeyPressed(Key.S)) Position -= _front * velocity;
        if (keyboard.IsKeyPressed(Key.A)) Position -= _right * velocity;
        if (keyboard.IsKeyPressed(Key.D)) Position += _right * velocity;
        if (keyboard.IsKeyPressed(Key.Space)) Position += _up * velocity;
        if (keyboard.IsKeyPressed(Key.ShiftLeft)) Position -= _up * velocity;
    }

    public void ProcessMouse(IMouse mouse)
    {
        var pos = new Vector2(mouse.Position.X, mouse.Position.Y);

        if (_firstMouse)
        {
            _lastMouse = pos;
            _firstMouse = false;
            return;
        }

        float dx = (pos.X - _lastMouse.X) * Sensitivity;
        float dy = (_lastMouse.Y - pos.Y) * Sensitivity;
        _lastMouse = pos;

        Yaw += dx;
        Pitch += dy;
        Pitch = Math.Clamp(Pitch, -89f, 89f);

        UpdateVectors();
    }

    private void UpdateVectors()
    {
        float yawRad = MathF.PI / 180f * Yaw;
        float pitchRad = MathF.PI / 180f * Pitch;

        _front = Vector3.Normalize(new Vector3(
            MathF.Cos(yawRad) * MathF.Cos(pitchRad),
            MathF.Sin(pitchRad),
            MathF.Sin(yawRad) * MathF.Cos(pitchRad)));

        _right = Vector3.Normalize(Vector3.Cross(_front, Vector3.UnitY));
        _up = Vector3.Normalize(Vector3.Cross(_right, _front));
    }

    public Matrix4X4<float> GetViewMatrix()
    {
        var target = Position + _front;
        return ToSilk(Matrix4x4.CreateLookAt(Position, target, _up));
    }

    public Matrix4X4<float> GetProjectionMatrix(float aspect)
    {
        return ToSilk(Matrix4x4.CreatePerspectiveFieldOfView(
            MathF.PI / 180f * Fov, aspect, 0.1f, 1000f));
    }

    private static Matrix4X4<float> ToSilk(Matrix4x4 m)
    {
        return new Matrix4X4<float>(
            m.M11, m.M12, m.M13, m.M14,
            m.M21, m.M22, m.M23, m.M24,
            m.M31, m.M32, m.M33, m.M34,
            m.M41, m.M42, m.M43, m.M44);
    }
}
