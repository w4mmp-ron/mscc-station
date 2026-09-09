# Linux USB-HID PSoC firmware upload (Multus / Proficio)
# Requires: libhidapi-dev (+ libusb if using hidapi-libusb)
#   sudo apt-get install -y build-essential libhidapi-dev libusb-1.0-0-dev

CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra
# gnu17: Ubuntu 26.04 gcc 15 defaults to C23, where foo() means foo(void).
CFLAGS  += -std=gnu17 -Icybootloaderutils -Isrc
LDFLAGS ?=
LIBS    := -lhidapi-libusb -lusb-1.0

# On some distros the pkg is hidapi-hidraw:
#   make LIBS="-lhidapi-hidraw -lusb-1.0"

SRCS = \
	src/main.c \
	src/hid_comm.c \
	cybootloaderutils/cybtldr_api.c \
	cybootloaderutils/cybtldr_api2.c \
	cybootloaderutils/cybtldr_command.c \
	cybootloaderutils/cybtldr_parse.c

OBJS = $(SRCS:.c=.o)
TARGET = bootloader

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(HOME)/mscc
	install -m 755 $(TARGET) $(DESTDIR)$(HOME)/mscc/
