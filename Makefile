#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#

EXTRA_INCLUDES	=

# Utilities
CP		= cp

# Build targets
MODULE		= dm
DMADM		= dmadm

CONFFILE	= $(MODULE).conf
MACH_32		=
MACH_64		= -m64 -xmodel=kernel
C99MODE		= -xc99=%all
KERNEL		= -D_KERNEL
INCLUDES	= -Iinclude $(EXTRA_INCLUDES)
CFLAGS		= -v $(KERNEL) $(C99MODE) $(INCLUDES)
CFLAGS_DMADM	= -v $(C99MODE) $(INCLUDES)
LDFLAGS		= -dy -Ndrv/blkdev
LINTFLAGS	= $(KERNEL) $(INCLUDES) -errsecurity=extended -Nlevel

SRCS		= dm.c
SRCS_DMADM	= dmadm.c

OBJS32		= $(SRCS:%.c=32/%.o)
OBJS64		= $(SRCS:%.c=64/%.o)

MODULE32	= 32/$(MODULE)
MODULE64	= 64/$(MODULE)

all: $(MODULE32) $(MODULE64) $(DMADM)

32 64:
	mkdir $@

32/%.o: %.c
	$(COMPILE.c) $(MACH_32) -o $@ $<

64/%.o: %.c
	$(COMPILE.c) $(MACH_64) -o $@ $<

$(MODULE32):	32 $(OBJS32)
	$(LD) -r -o $(MODULE32) $(LDFLAGS) $(OBJS32)

$(MODULE64):	64 $(OBJS64)
	$(LD) -r -o $(MODULE64) $(LDFLAGS) $(OBJS64)

$(DMADM):	$(SRCS_DMADM)
	$(CC) $(CFLAGS_DMADM) -o $(DMADM) $(SRCS_DMADM)

install_files: $(CONFFILE) $(MODULE32) $(MODULE64)
	pfexec $(CP) $(CONFFILE) /usr/kernel/drv
	pfexec $(CP) $(MODULE32) /usr/kernel/drv
	pfexec $(CP) $(MODULE64) /usr/kernel/drv/amd64

install: install_files
	pfexec add_drv -v -n $(MODULE)

devlink:
	echo "type=ddi_pseudo;name=dm\tdm\M0\tmapper/control" >> /etc/devlink.tab

lint:
	$(LINT.c) $(SRCS)

cstyle:
	cstyle -chpPv $(SRCS) dm.h

clean:
	$(RM) $(DMADM) $(MODULE32) $(MODULE64) $(OBJS32) $(OBJS64)
