namespace UniversalSim2;

/// <summary>
/// Main menu state. In Vulkan mode, the menu is keyboard-driven.
/// The star scene renders as background.
/// Controls are printed to console on startup.
/// </summary>
public static class MainMenu
{
    public static void PrintInstructions()
    {
        Console.WriteLine("========================================");
        Console.WriteLine("       UNIVERSAL SIM 2 — v0.0.2        ");
        Console.WriteLine("========================================");
        Console.WriteLine();
        Console.WriteLine("  [ENTER]  — PLAY");
        Console.WriteLine("  [Q]      — QUIT");
        Console.WriteLine();
        Console.WriteLine("  In-game controls:");
        Console.WriteLine("  WASD     — Move");
        Console.WriteLine("  Mouse    — Look around");
        Console.WriteLine("  Space    — Up");
        Console.WriteLine("  Shift    — Down");
        Console.WriteLine("  ESC      — Unlock cursor / Menu");
        Console.WriteLine("========================================");
    }
}
