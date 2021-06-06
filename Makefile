TARGET = umd_dump
PSP_FW_VERSION = 500
BUILD_PRX = 1

#USE_KERNEL_LIBC = 1

#USE_KERNEL_LIBS = 1

CLASSG_LIBS = libs
#CLASSG_LIBS = libc

INCDIR = $(CLASSG_LIBS)
CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

LIBS = -lpspreg -lpspumd -lpsprtc -lpspsystemctrl_kernel -lpspkubridge

OBJS = 	scepower.o main.o

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
