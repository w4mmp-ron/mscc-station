# sdrcore-recv Linux / Pi / WSL — full PortAudio audio path (no stubs)
#
# Prefer system MSCC PortAudio (/usr/local) or $PORTAUDIO_PREFIX.
# Without -L/-rpath, -lportaudio often binds Trixie /lib/aarch64-linux-gnu.
#
# Operator play may open at device rate; I/Q stays 96 kHz. Oboe freestanding
# MultiChannelResampler (sources/resampler/) bridges AF when rates differ.
#
#   sudo apt-get install -y build-essential g++
#   # PortAudio+Pulse already installed to /usr/local (see portaudio/BUILD-MSCC.txt)
#   cd /path/to/SDRcore-recv-linux
#   make clean && make
#   ldd $(BINDIR)/sdrcore-recv | grep portaudio   # expect /usr/local/lib
#
# Expects Multus/Proficio PortAudio I/Q capture + speaker via operator-speaker.ini

CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -O2 -g -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
CFLAGS  += -iquote sources -D__linux__ -DRON
CXXFLAGS ?= -O2 -g -Wall -std=c++17
# Use -iquote only (not -Isources): sources/pthread.h would shadow system pthread
# and break libstdc++ when compiling the Oboe resampler.
CXXFLAGS += -iquote sources -iquote sources/resampler -DRESAMPLER_OUTER_NAMESPACE=mscc
LDFLAGS ?=
LIBS    := -lpthread -lm -lstdc++

# MSCC custom PortAudio (Pulse+ALSA). Override: make PORTAUDIO_PREFIX=/opt/...
# Deb may install only libportaudio.so.2 (no unversioned .so) — accept either.
PORTAUDIO_PREFIX ?= /usr/local

PA_LIBDIR :=
ifneq ($(wildcard $(PORTAUDIO_PREFIX)/lib/libportaudio.so*),)
  PA_LIBDIR := $(PORTAUDIO_PREFIX)/lib
else ifneq ($(wildcard $(PORTAUDIO_PREFIX)/lib/aarch64-linux-gnu/libportaudio.so*),)
  PA_LIBDIR := $(PORTAUDIO_PREFIX)/lib/aarch64-linux-gnu
endif

ifneq ($(PA_LIBDIR),)
  CFLAGS  += -I$(PORTAUDIO_PREFIX)/include
  CXXFLAGS += -I$(PORTAUDIO_PREFIX)/include
  LDFLAGS += -L$(PA_LIBDIR) -Wl,-rpath,$(PA_LIBDIR)
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := PortAudio at $(PA_LIBDIR) (rpath set)
else
  LIBS    := -lportaudio $(LIBS)
  PA_NOTE := WARNING: no libportaudio at $(PORTAUDIO_PREFIX)/lib — using default -lportaudio (often distro)
endif

BINDIR  ?= $(HOME)/mscc

# Freestanding Oboe resampler (vendored under sources/resampler/)
RESAMPLER_SRCS = \
	sources/resampler/IntegerRatio.cpp \
	sources/resampler/LinearResampler.cpp \
	sources/resampler/MultiChannelResampler.cpp \
	sources/resampler/PolyphaseResampler.cpp \
	sources/resampler/PolyphaseResamplerMono.cpp \
	sources/resampler/PolyphaseResamplerStereo.cpp \
	sources/resampler/SincResampler.cpp \
	sources/resampler/SincResamplerStereo.cpp

SRCS = \
	sources/platform_linux.c \
	sources/blanker.c \
	sources/dsputils.c \
	sources/filter.c \
	sources/flusher.c \
	sources/iq.c \
	sources/main.c \
	sources/panadapter.c \
	sources/print-utils.c \
	sources/remote_phones.c \
	sources/sdrcore.c \
	sources/output-devices.c \
	sources/tonegen.c \
	sources/udp_thread.c \
	sources/wsfirgen.c

CXX_SRCS = \
	sources/mscc_resampler.cpp \
	$(RESAMPLER_SRCS)

OBJS = $(SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
TARGET = $(BINDIR)/sdrcore-recv

.PHONY: all clean

all: $(TARGET)
	@echo "  $(PA_NOTE)"
	@echo "  built: $(TARGET)"
	@ldd $(TARGET) 2>/dev/null | grep -i portaudio || true

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

sources/%.o: sources/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

sources/%.o: sources/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

sources/resampler/%.o: sources/resampler/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
