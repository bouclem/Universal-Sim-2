namespace UniversalSim2;

internal static class Program
{
    [System.STAThread]
    public static void Main()
    {
        MainMenu.PrintInstructions();
        Game.Run();
    }
}
