savedcmd_probe.mod := printf '%s\n'   probe.o | awk '!x[$$0]++ { print("./"$$0) }' > probe.mod
