#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#

EXTRA_INCLUDES	=

CP		= cp
MODULE		= dm
CONFFILE	= $(MODULE).conf
MACH_64		= -m64 -xmodel=kernel
C99MODE		= -xc99=%all
KERNEL		= -D_KERNEL
INCLUDES	= -Iinclude $(EXTRA_INCLUDES)
CFLAGS		= -v $(KERNEL) $(C99MODE) $(INCLUDES)
LDFLAGS		= -dy -Ndrv/blkdev
LINTFLAGS	= $(KERNEL) $(INCLUDES) -errsecurity=extended -Nlevel
SRCS		= dm.c

OBJS32		= $(SRCS:%.c=32/%.o)
OBJS64		= $(SRCS:%.c=64/%.o)

TARGET32	= 32/$(MODULE)
TARGET64	= 64/$(MODULE)

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

install_files: $(CONFFILE) $(TARGET32) $(TARGET64)
	pfexec $(CP) $(CONFFILE) /usr/kernel/drv
	pfexec $(CP) $(TARGET32) /usr/kernel/drv
	pfexec $(CP) $(TARGET64) /usr/kernel/drv/amd64

install: install_files
	pfexec add_drv -v -n $(MODULE)

devlink:
	echo "type=ddi_pseudo;name=dm\tdm\M0\tmapper/control" >> /etc/devlink.tab

lint:
	$(LINT.c) $(SRCS)

cstyle:
	cstyle -chpPv $(SRCS) dm.h

clean:
	$(RM) $(TARGET32) $(TARGET64) $(OBJS32) $(OBJS64)
