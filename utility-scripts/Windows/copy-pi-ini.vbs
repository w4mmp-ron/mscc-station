' Double-click: copy the safe radio settings from the Pi to the Windows servers
' with no console window (runs copy-pi-ini.ps1 -Gui; message boxes only).
Set fso = CreateObject("Scripting.FileSystemObject")
ps1 = fso.BuildPath(fso.GetParentFolderName(WScript.ScriptFullName), "copy-pi-ini.ps1")
CreateObject("WScript.Shell").Run "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File """ & ps1 & """ -Gui", 0, False
