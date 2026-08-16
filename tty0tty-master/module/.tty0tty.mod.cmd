savedcmd_tty0tty.mod := printf '%s\n'   tty0tty.o | awk '!x[$$0]++ { print("./"$$0) }' > tty0tty.mod
