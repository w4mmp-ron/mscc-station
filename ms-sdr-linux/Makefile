# ms-sdr Linux build (Pi OS / arm64 — real Proficio USB)
#
#   make
#   # binary: $(BINDIR)/ms-sdr
#
# Needs: libusb-1.0-0-dev, build-essential
#

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wno-unused-variable -Wno-unused-but-set-variable
# -iquote so "extern.h" resolves under source/, but <pthread.h> stays system
# (plain -I source shadows system pthread with vendored Win32 pthread.h)
CFLAGS  += -iquote source -D__linux__
LDFLAGS ?=
LIBS    := -lpthread -lm -lusb-1.0 -lutil

# Install / run directory for all MSCC Linux binaries
BINDIR  ?= $(HOME)/mscc

SRCS = \
	source/platform_linux.c \
	source/amplifier.c \
	source/calibrate.c \
	source/comm-port.c \
	source/crc.c \
	source/cw-snap.c \
	source/freq_queue.c \
	source/iq.c \
	source/last_used.c \
	source/main-controller.c \
	source/main.c \
	source/message_queue.c \
	source/power_calibration.c \
	source/proficio-temperature.c \
	source/usbcmd.c \
	source/user_controls.c

OBJS = $(SRCS:.c=.o)
TARGET = $(BINDIR)/ms-sdr
VERSION_H = source/version.h

.PHONY: all clean bump-version

# Bump once, then build (recursive make avoids bumping once per .o).
all:
	@$(MAKE) bump-version
	@$(MAKE) $(TARGET)

# Auto-increment VERSION_MINOR each build (Windows: PreBuildEvent).
bump-version:
	@chmod +x "$(CURDIR)/bump-version.sh" 2>/dev/null || true
	@"$(CURDIR)/bump-version.sh"

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# version.h is a dep so Core version change forces recompile of includers.
source/%.o: source/%.c $(VERSION_H)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
