' Launch a program fully hidden, with an explicit working directory.
' Usage:
'   cscript //nologo runhidden.vbs <workingDir> <exePath> [args...]
Option Explicit
Dim sh, workDir, exePath, cmd, i
If WScript.Arguments.Count < 2 Then
  WScript.Echo "Usage: runhidden.vbs <workingDir> <exePath> [args...]"
  WScript.Quit 1
End If

workDir = WScript.Arguments(0)
exePath = WScript.Arguments(1)

Set sh = CreateObject("Wscript.Shell")
sh.CurrentDirectory = workDir

cmd = """" & exePath & """"
For i = 2 To WScript.Arguments.Count - 1
  cmd = cmd & " " & WScript.Arguments(i)
Next

' 0 = hidden window, False = do not wait
sh.Run cmd, 0, False
