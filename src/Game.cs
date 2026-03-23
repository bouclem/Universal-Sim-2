using System.Numerics;
using Silk.NET.Input;
using Silk.NET.Maths;
using Silk.NET.Windowing;
using UniversalSim2.Rendering;

namespace UniversalSim2;

public enum GameState
{
    MainMenu,
    Playing
}

public static class Game
{
    public const string Title = "Universal Sim 2";
    public static GameState State = GameState.MainMenu;

    private static IWindow _window = null!;
    private static IInputContext _input = null!;
    private static IKeyboard _keyboard = null!;
    private static IMouse _mouse = null!;
    private static VulkanEngine _vulkan = null!;
    private static Camera _camera = null!;

    private static DateTime _lastFrame;
    private static bool _cursorLocked;

    public static void Run()
    {
        var opts = WindowOptions.DefaultVulkan;
        opts.Title = Title;
        opts.WindowState = WindowState.Fullscreen;
        opts.VSync = true;

        _window = Window.Create(opts);
        _window.Load += OnLoad;
        _window.Render += OnRender;
        _window.Closing += OnClose;
        _window.Run();
    }

    private static void OnLoad()
    {
        _input = _window.CreateInput();
        _keyboard = _input.Keyboards[0];
        _mouse = _input.Mice[0];

        _keyboard.KeyDown += OnKeyDown;

        _camera = new Camera(new Vector3(0, 2, 8));
        _lastFrame = DateTime.UtcNow;

        _vulkan = new VulkanEngine(_window);
        _vulkan.Initialize();

        // Upload star mesh
        GameScene.Initialize();
        _vulkan.UploadMesh(GameScene.GetVertices(), GameScene.GetIndices());
    }

    private static void OnKeyDown(IKeyboard kb, Key key, int scancode)
    {
        if (key == Key.Escape)
        {
            if (State == GameState.Playing)
            {
                // Toggle cursor lock or go back to menu
                if (_cursorLocked)
                {
                    _cursorLocked = false;
                    _mouse.Cursor.CursorMode = CursorMode.Normal;
                }
                else
                {
                    State = GameState.MainMenu;
                }
            }
            else
            {
                _window.Close();
            }
        }

        if (State == GameState.MainMenu)
        {
            if (key == Key.Enter || key == Key.KeypadEnter)
            {
                State = GameState.Playing;
                _cursorLocked = true;
                _mouse.Cursor.CursorMode = CursorMode.Raw;
            }
            if (key == Key.Q)
            {
                _window.Close();
            }
        }
    }

    private static void OnRender(double deltaTime)
    {
        var now = DateTime.UtcNow;
        float dt = (float)(now - _lastFrame).TotalSeconds;
        _lastFrame = now;

        if (State == GameState.Playing)
        {
            if (_cursorLocked)
            {
                _camera.ProcessKeyboard(_keyboard, dt);
                _camera.ProcessMouse(_mouse);
            }

            float aspect = (float)_window.Size.X / _window.Size.Y;
            var view = _camera.GetViewMatrix();
            var proj = _camera.GetProjectionMatrix(aspect);

            // Vulkan clip space: flip Y
            var clip = new Matrix4X4<float>(
                1, 0, 0, 0,
                0, -1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);

            var model = Matrix4X4<float>.Identity;
            var mvp = Multiply(Multiply(model, view), Multiply(proj, clip));

            _vulkan.UpdateUniforms(mvp);
            _vulkan.DrawFrame();
        }
        else
        {
            // In menu state, still draw the scene as background
            float aspect = (float)_window.Size.X / _window.Size.Y;
            var view = _camera.GetViewMatrix();
            var proj = _camera.GetProjectionMatrix(aspect);
            var clip = new Matrix4X4<float>(
                1, 0, 0, 0,
                0, -1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
            var model = Matrix4X4<float>.Identity;
            var mvp = Multiply(Multiply(model, view), Multiply(proj, clip));
            _vulkan.UpdateUniforms(mvp);
            _vulkan.DrawFrame();
        }
    }

    private static Matrix4X4<float> Multiply(Matrix4X4<float> a, Matrix4X4<float> b)
    {
        return new Matrix4X4<float>(
            a.M11*b.M11 + a.M12*b.M21 + a.M13*b.M31 + a.M14*b.M41,
            a.M11*b.M12 + a.M12*b.M22 + a.M13*b.M32 + a.M14*b.M42,
            a.M11*b.M13 + a.M12*b.M23 + a.M13*b.M33 + a.M14*b.M43,
            a.M11*b.M14 + a.M12*b.M24 + a.M13*b.M34 + a.M14*b.M44,
            a.M21*b.M11 + a.M22*b.M21 + a.M23*b.M31 + a.M24*b.M41,
            a.M21*b.M12 + a.M22*b.M22 + a.M23*b.M32 + a.M24*b.M42,
            a.M21*b.M13 + a.M22*b.M23 + a.M23*b.M33 + a.M24*b.M43,
            a.M21*b.M14 + a.M22*b.M24 + a.M23*b.M34 + a.M24*b.M44,
            a.M31*b.M11 + a.M32*b.M21 + a.M33*b.M31 + a.M34*b.M41,
            a.M31*b.M12 + a.M32*b.M22 + a.M33*b.M32 + a.M34*b.M42,
            a.M31*b.M13 + a.M32*b.M23 + a.M33*b.M33 + a.M34*b.M43,
            a.M31*b.M14 + a.M32*b.M24 + a.M33*b.M34 + a.M34*b.M44,
            a.M41*b.M11 + a.M42*b.M21 + a.M43*b.M31 + a.M44*b.M41,
            a.M41*b.M12 + a.M42*b.M22 + a.M43*b.M32 + a.M44*b.M42,
            a.M41*b.M13 + a.M42*b.M23 + a.M43*b.M33 + a.M44*b.M43,
            a.M41*b.M14 + a.M42*b.M24 + a.M43*b.M34 + a.M44*b.M44);
    }

    private static void OnClose()
    {
        _vulkan?.Dispose();
        _input?.Dispose();
    }
}
