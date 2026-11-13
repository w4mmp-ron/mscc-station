namespace ProficioDfuDownloader;

static class Program
{
    [STAThread]
    static void Main()
    {
        ApplicationConfiguration.Initialize();
        var running = MainForm.ServersRunning();
        if (running.Count > 0)      // no firmware update while the servers run
        {
            MessageBox.Show(MainForm.ServersMessage(running), "STM32 App Flash (Black Pill, USB DFU)",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }
        Application.Run(new MainForm());
    }
}
