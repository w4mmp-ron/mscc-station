# mscc-init Linux — interactive MSCC base config (libusb serial + PortAudio devices)
#
# Prefer system MSCC PortAudio (/usr/local) or $PORTAUDIO_PREFIX — same as
# sdrcore-recv / sdrcore-trans. Without -L/-rpath, -lportaudio often binds
# Trixie /lib/aarch64-linux-gnu (ALSA-only / no Virtual*).
#
#   sudo apt-get install -y build-essential libusb-1.0-0-dev
#   # PortAudio+Pulse already at /usr/local (see portaudio/BUILD-MSCC.txt)
#   cd /path/to/mscc-init-linux
#   make clean && make
#   ldd $(BINDIR)/mscc-init | grep portaudio   # expect /usr/local/lib
#
# Writes $HOME/.local/mscc/: mscc.ini, cw.ini, i2c.ini, comm-port.ini, speaker/mic inis

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wno-unused-variable
# gnu17: Ubuntu 26.04 gcc 15 defaults to C23, where foo() means foo(void).
CFLAGS  += -std=gnu17 -iquote sources -D__linux__
LDFLAGS ?=
LIBS    := -lusb-1.0 -lpthread -lm

# Audio link (same sources, different host):
#   PORTAUDIO=mscc   (default)  rpath /usr/local — Pi mscc-portaudio
#   PORTAUDIO=distro            system -lportaudio — Ubuntu libportaudio2
PORTAUDIO ?= mscc
PORTAUDIO_PREFIX ?= /usr/local

ifeq ($(PORTAUDIO),distro)
  LIBS := -lportaudio $(LIBS)
  PA_NOTE := PortAudio: distro (-lportaudio, no rpath)
else ifneq ($(wildcard $(PORTAUDIO_PREFIX)/lib/libportaudio.so),)
  CFLAGS  += -I$(PORTAUDIO_PREFIX)/include
  PA_RPATH := $(if $(PORTAUDIO_RPATH),$(PORTAUDIO_RPATH),$(PORTAUDIO_PREFIX)/lib)
  LDFLAGS += -L$(PORTAUDIO_PREFIX)/lib -Wl,-rpath,$(PA_RPATH)
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := PortAudio at $(PORTAUDIO_PREFIX) (rpath $(PA_RPATH))
else ifneq ($(wildcard $(PORTAUDIO_PREFIX)/lib/aarch64-linux-gnu/libportaudio.so),)
  PALIB := $(PORTAUDIO_PREFIX)/lib/aarch64-linux-gnu
  CFLAGS  += -I$(PORTAUDIO_PREFIX)/include
  PA_RPATH := $(if $(PORTAUDIO_RPATH),$(PORTAUDIO_RPATH),$(PALIB))
  LDFLAGS += -L$(PALIB) -Wl,-rpath,$(PA_RPATH)
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := PortAudio at $(PALIB) (rpath $(PA_RPATH))
else ifneq ($(wildcard $(PORTAUDIO_PREFIX)/lib/libportaudio.a),)
  CFLAGS  += -I$(PORTAUDIO_PREFIX)/include
  LDFLAGS += -L$(PORTAUDIO_PREFIX)/lib
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := static PortAudio at $(PORTAUDIO_PREFIX)
else
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := WARNING: no lib at $(PORTAUDIO_PREFIX)/lib — using default -lportaudio (often distro)
endif

BINDIR  ?= $(HOME)/mscc

SRCS = sources/platform_linux.c sources/main.c
OBJS = $(SRCS:.c=.o)
TARGET = $(BINDIR)/mscc-init

.PHONY: all clean

all: $(TARGET)
	@echo "  $(PA_NOTE)"
	@echo "  built: $(TARGET)"
	@ldd $(TARGET) 2>/dev/null | grep -i portaudio || true

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

sources/%.o: sources/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
