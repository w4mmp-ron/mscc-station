# sdrcore-trans Linux / Pi / WSL — full PortAudio audio path (no stubs)
#
# Prefer system MSCC PortAudio (/usr/local) or $PORTAUDIO_PREFIX.
# Operator mic may open at device rate; Proficio I/Q stays 96 kHz.
# Oboe freestanding MultiChannelResampler (sources/resampler/) upsamples mic AF.
#
#   make clean && make
#   ldd $(BINDIR)/sdrcore-trans | grep portaudio   # expect /usr/local/lib

CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -O2 -g -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
CFLAGS  += -iquote sources -D__linux__ -DRON
CXXFLAGS ?= -O2 -g -Wall -std=c++17
# -iquote only: sources/pthread.h must not shadow system pthread for libstdc++
CXXFLAGS += -iquote sources -iquote sources/resampler -DRESAMPLER_OUTER_NAMESPACE=mscc
LDFLAGS ?=
LIBS    := -lpthread -lm -lstdc++

# MSCC custom PortAudio. Deb may install only libportaudio.so.2 — accept either.
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
	sources/amplifier.c \
	sources/driver.c \
	sources/dsputils.c \
	sources/filter.c \
	sources/flusher.c \
	sources/input-devices.c \
	sources/iq.c \
	sources/main.c \
	sources/overdriven.c \
	sources/print-utils.c \
	sources/sdrcoretx.c \
	sources/power.c \
	sources/udp_thread.c \
	sources/remote_mic.c \
	sources/alc.c \
	sources/wsfirgen.c

CXX_SRCS = \
	sources/mscc_resampler.cpp \
	$(RESAMPLER_SRCS)

OBJS = $(SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
TARGET = $(BINDIR)/sdrcore-trans

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
