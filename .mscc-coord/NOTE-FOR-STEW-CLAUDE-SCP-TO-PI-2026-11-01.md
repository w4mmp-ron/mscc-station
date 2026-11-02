# For Stew: letting Claude Code reach your Pi with ssh and scp (no Samba, no Linux skills needed)

From Ron / Claude, 2026-11-01.

## The whole picture on one page

```
                         GitHub (on the internet)
                  The shared copy of the code. You and Ron
                  each "pull" to get the other's changes and
                  "push" to send yours. Nothing else goes here.
                                   ^
                                   |  git pull / git push
                                   |
   +-------------------------------+--------------------------------+
   |                     YOUR WINDOWS PC                            |
   |                                                                |
   |   Claude Code runs HERE (installed on this PC, not the         |
   |   browser version). Your copy of the repo is here.             |
   |   You build MSCC UI and the Windows servers here.              |
   |                                                                |
   |   Your ssh key lives here: one private file, never shared.     |
   +-----------------+------------------------------+---------------+
                     |                              |
          ssh + scp  |  over your home network      |  ssh + scp
  (copy files, build,|  (no Samba, no shared drive) |  (copy MSCC UI,
   read logs)        |                              |   run commands)
                     v                              v
   +----------------------------------+   +----------------------------------+
   |            YOUR PI               |   |          YOUR UBUNTU PC          |
   |                                  |   |                                  |
   |  ssh already on (you use it now) |   |  needs ssh server added ONCE:    |
   |  Has your public key (copied     |   |   sudo apt install openssh-server|
   |  once, no more passwords).       |   |  Has your public key (copied     |
   |  Servers built here with make.   |   |  once, no more passwords).       |
   |  Logs in ~/.local/mscc/          |   |  MSCC UI copied here to test.    |
   +----------------------------------+   +----------------------------------+

   Claude Code is installed on ONE machine only: the Windows PC.
   The Pi and the Ubuntu PC only need ssh and your key. Nothing else.
   You do restarts and anything that transmits; Claude builds, copies and reads logs.
```

## Why this note

Tonight Ron and Claude did a whole round of Pi work in one sitting: port 2 TONE, build it on the
Pi, read the server logs during the on-air tests, fix the SWR meter, build again, check again.
It went quickly because Claude Code, running on Ron's Windows PC, can reach the Pi directly. It
copies the changed source files to the Pi, runs `make` there, restarts nothing on its own, and
reads the logs (`~/.local/mscc/ms-sdr.log` and the others) to see what happened.

Ron's station does part of this through a Samba share (the Pi's home folder shows up as a drive
letter on Windows). Ron doesn't want you to have to set that up: it takes more Linux knowledge
than it's worth. **You don't need it.** Windows 10 and 11 already have the same tools Linux uses
(`ssh`, `scp` and `sftp`), and those are all Claude needs. Ron is fairly sure you already use ssh to
reach your Pi, so most of the setup is probably done.

## Important: this needs Claude Code running on your PC, not in the browser

Ron mentioned you have been using Claude Code in the browser (claude.ai/code). That version runs
on Anthropic's computers in the cloud, not on yours, with a copy of the GitHub repo. It can read, change and push code, but
it can't reach anything on your home network: not your Pi, not your radio, not your meter, not
your Windows build tools. Everything below needs Claude Code installed on your PC: the Windows
desktop app, or the command-line version run from PowerShell. That is how Ron runs it.

## What the three tools do, in plain words

- **ssh** runs a command on the Pi from your PC and shows the output, for example "build ms-sdr" or
  "show me the last 50 lines of the ms-sdr log". You already know this one.
- **scp** copies a file from your PC to the Pi, or back. One command per copy.
- **sftp** does the same job as scp, but is handier when there are many files to copy at once.
  Claude would only use it for that; scp is usually enough.

All three use the same login, so once ssh works without a password, scp and sftp do too.

## The one thing that matters: a key instead of a password

Claude can run commands, but it can't type a password when one is asked for. If your ssh asks for your
Pi password every time, Claude's ssh and scp commands will just hang or fail. The fix is an
**ssh key**: a pair of small files that prove who you are without typing anything. This is the
normal, secure way to use ssh, and it's set up once.

**How to tell whether you already have one:** open PowerShell and type `ssh pi@192.168.x.x`, using
your Pi's user name and address. If you are logged in to the Pi without being asked for a password,
you already have a working key. Skip to "Tell Claude about your Pi".

**If it asks for a password**, do this once (about two minutes):

1. Open PowerShell (an ordinary window, not as administrator).
2. Make the key:
   ```
   ssh-keygen -t ed25519
   ```
   It asks where to save it: press Enter to accept the normal place
   (`C:\Users\<you>\.ssh\id_ed25519`). It asks for a passphrase: press Enter twice for none.
   (With a passphrase, Claude would be stuck waiting for it to be typed, just like a password.)
3. Copy the public half of the key to the Pi. In this one command, change `pi` and the address to
   your Pi's user name and address:
   ```
   type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh pi@192.168.x.x "mkdir -p ~/.ssh; chmod 700 ~/.ssh; cat >> ~/.ssh/authorized_keys; chmod 600 ~/.ssh/authorized_keys"
   ```
   It asks for your Pi password **one last time**.
4. Test it:
   ```
   ssh pi@192.168.x.x "echo hello from the Pi"
   ```
   It should print `hello from the Pi` without asking for a password. Done.

About safety: the file ending in `.pub` is the public half and is safe to put on the Pi. The other
file (`id_ed25519`, no extension) is the private half: it stays on your PC and must never be
copied anywhere or put in the repo.

## Tell Claude about your Pi

Claude doesn't know your Pi's address, user name or folders. Write them down once where Claude
reads them at the start of every session: your `CLAUDE.md` (Ron's is `rpi/CLAUDE.md`; yours is
`mscc-ui/windows-work-tree/CLAUDE.md`). Something like this, with your own values:

```
## My Pi (for builds and logs)
- ssh pi@192.168.x.x (key login, no password)
- Server sources on the Pi: ~/ms-sdr-linux, ~/SDRcore-trans-linux, ~/SDRcore-recv-linux
- Build: cd <folder> && make clean && make   (installs to ~/mscc/: replaces the program file, but the running copy keeps running until a restart)
- Logs: ~/.local/mscc/ms-sdr.log, sdrcore-trans.log, sdrcore-recv.log
- Restart MSCC: only when I say so (mscc stop; mscc start)
```

If your Pi folders are laid out differently from Ron's, just write down what you actually have.
Claude can also look around with ssh (`ls`) and tell you what it finds.

## What a working session then looks like

You ask Claude something like "port this change to the Pi and build it". Claude then:

1. Edits the source files on your PC, in the repo.
2. Copies only the changed files to the Pi, for example:
   ```
   scp rpi/SDRcore-trans-linux/sources/main.c pi@192.168.x.x:~/SDRcore-trans-linux/sources/
   ```
3. Builds on the Pi:
   ```
   ssh pi@192.168.x.x "cd ~/SDRcore-trans-linux && make clean && make"
   ```
   and reads the compiler output for errors and new warnings.
4. If the build changed a file on the Pi that belongs in the repo (ms-sdr raises the version number in its own
   `version.h` on every build), copies that file back with scp.
5. After you test on the air, reads the logs over ssh and tells you what they show.

You stay in charge of the station. A sensible rule, and the one Ron uses: Claude may build, but
it does not restart MSCC or do anything that transmits. You do the restart and the on-air tests,
then ask Claude to check the logs.

## Your Ubuntu PC: a second machine Claude can copy to, just like the Pi

You build MSCC UI (Avalonia) on your Windows PC and then move it to the Pi and to the Ubuntu PC.
The same tools work for both. **Claude Code stays on your Windows PC only.** There is no need to
install it on the Ubuntu PC. From Windows, Claude can copy the new build to the Ubuntu PC with
scp, and run commands there with ssh, exactly as it does with the Pi. The Ubuntu PC is just a
second address in the commands.

There is one difference from the Pi: **Ubuntu Desktop does not come with the ssh "server" part
installed.** The Pi has it (once SSH is turned on), but a normal Ubuntu Desktop install can only
make ssh connections OUT to other machines; it can't accept them coming IN. So the Ubuntu PC needs
one small program added, once.

**On the Ubuntu PC, once** (in a terminal there):

1. Install the ssh server:
   ```
   sudo apt install openssh-server
   ```
   It asks for your Ubuntu password (that's `sudo`, normal for installing). It starts by itself
   and starts again at every boot; there is nothing else to configure.
2. Find the Ubuntu PC's address:
   ```
   hostname -I
   ```
   The first number shown (something like `192.168.1.57`) is the one to use.

**Then on your Windows PC**, the same two steps you did for the Pi, but with the Ubuntu PC's user
name and address. You don't need a new key: the key you already made on Windows works for any
number of machines.

1. Copy your public key to the Ubuntu PC (change `stew` and the address to your Ubuntu user name
   and the address from step 2):
   ```
   type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh stew@192.168.x.x "mkdir -p ~/.ssh; chmod 700 ~/.ssh; cat >> ~/.ssh/authorized_keys; chmod 600 ~/.ssh/authorized_keys"
   ```
   It asks for your Ubuntu password one last time (and the first time it also asks whether you
   trust that computer: answer `yes`).
2. Test it:
   ```
   ssh stew@192.168.x.x "echo hello from Ubuntu"
   ```
   It should print `hello from Ubuntu` without asking for a password.

**Tell Claude.** Add the Ubuntu PC to the same CLAUDE.md block, under the Pi, for example:

```
## My Ubuntu PC (MSCC UI test machine)
- ssh stew@192.168.x.x (key login, no password)
- MSCC UI goes to: ~/<the folder you use>
```

Write down the folder you actually copy MSCC UI into, so Claude puts it in the same place you do.

After that you can say things like "build MSCC UI and copy it to the Pi and the Ubuntu PC", and
Claude can do the copying for both, then check that the files arrived.

**Seeing the files yourself** (optional): WinSCP on Windows (next section) opens the Ubuntu PC the
same way it opens the Pi; it's just another saved connection.

## If you'd also like to see the Pi's files yourself (Windows)

**WinSCP** (free, winscp.net) shows the Pi's folders in a Windows Explorer-style window and lets
you drag files back and forth. It uses the same SFTP connection and can use the same key (it will
offer to convert it on first use). Claude doesn't need it; it's just handy for you.

## Things that can trip you up

- **"Permission denied (publickey)"**: the key wasn't copied, or went to a different user than the
  one in the command. Repeat step 3 with the right user name.
- **A "do you trust this computer?" question the first time** (ssh calls it the "host key"): ssh
  asks once whether you trust the Pi.
  Answer `yes` yourself in PowerShell before letting Claude use it, so Claude doesn't get stuck on
  that question.
- **The Pi changes its address**: Claude's commands stop working. A fixed address for the Pi in
  your router avoids this (most routers call it a "reserved" or "static" address for a device).
- **The Pi's name doesn't work, only its number**: on Ron's PC the name `proficio` works in PowerShell but not
  in Claude's bash shell. Using the IP address avoids that completely.

## Summary

- Install and use Claude Code on your Windows PC (not the browser version). That one PC is
  enough; it reaches both the Pi and the Ubuntu PC.
- Make an ssh key on Windows once (`ssh-keygen`), and copy it to the Pi and to the Ubuntu PC with
  the `type ... | ssh` line. Test that `ssh user@<address>` works without a password for each.
- The Ubuntu PC needs `sudo apt install openssh-server` once; the Pi already has ssh.
- Write both machines' addresses, users and folders into your CLAUDE.md.
- Claude can then copy files with scp, build with ssh and read the logs, the same way it does at
  Ron's station, with no Samba share and no Linux knowledge needed on your side.
