namespace MSCC.Remote;

static class Program
{
    /// <summary>
    /// No args → WinForms menu.
    /// Args → silent CLI (start|stop|restart|status|legacy|mkii|keyer) for Task Scheduler / scripts.
    /// </summary>
    [STAThread]
    static async Task<int> Main(string[] args)
    {
        if (args.Length > 0)
            return await ServerManager.RunCliAsync(args);

        ApplicationConfiguration.Initialize();
        Application.Run(new MainForm());
        return 0;
    }
}
