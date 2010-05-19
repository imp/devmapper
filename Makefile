#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#

EXTRA_INCLUDES	= -I/data/opensolaris/onnv-gate/usr/src/uts/common

MACH_64		= -m64 -xmodel=kernel
C99MODE		= -xc99=%all
CFLAGS		= -v -D_KERNEL $(C99MODE) $(EXTRA_INCLUDES)
LDFLAGS		= -dy -Ndrv/blkdev

SRCS		= dm.c

OBJS32		= $(SRCS:%.c=32/%.o)
OBJS64		= $(SRCS:%.c=64/%.o)

TARGET32	= 32/dm
TARGET64	= 64/dm

all: $(TARGET32) $(TARGET64)

32 64:
	mkdir $@

32/%.o: %.c
	$(COMPILE.c) $(MACH_32) -o $@ $<

64/%.o: %.c
	$(COMPILE.c) $(MACH_64) -o $@ $<

$(TARGET32):	32 $(OBJS32)
	$(LD) -r -o $(TARGET32) $(LDFLAGS) $(OBJS32)

$(TARGET64):	64 $(OBJS64)
	$(LD) -r -o $(TARGET64) $(LDFLAGS) $(OBJS64)

clean:
	$(RM) $(TARGET32) $(TARGET64) $(OBJS32) $(OBJS64)
