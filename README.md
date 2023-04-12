# SnowProcess
TL;DR:
❄️whitelist manager which mange which processes allowed to run and which ain't❄️ 

Whitelist kernel-mode Antivirus based on predetermined list of executable names, written in CPP using Windows Driver Kit.
The Driver relies on a Call-back which filters the process creation, protects Antivirus's config file using Minifilters and communicates with the driver using IOCTL.
