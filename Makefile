#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#
.KEEP_STATE:

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
DEBUG		= -g -xO0
KERNEL		= -D_KERNEL
INCLUDES	= -I./include $(EXTRA_INCLUDES)
CFLAGS		= -v $(DEBUG) $(KERNEL) $(C99MODE) $(INCLUDES)
CFLAGS_DMADM	= -v $(DEBUG) $(C99MODE) $(INCLUDES)
LDFLAGS		=
LINTFLAGS	= $(KERNEL) $(INCLUDES) -errsecurity=extended -Nlevel

HDRS		= include/sys/dm.h include/sys/dm_impl.h
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
	$(LD) -r -o $@ $(LDFLAGS) $(OBJS32)

$(MODULE64):	64 $(OBJS64)
	$(LD) -r -o $@ $(LDFLAGS) $(OBJS64)

$(DMADM):	$(SRCS_DMADM)
	$(CC) $(MACH_64) $(CFLAGS_DMADM) -o $@ $(SRCS_DMADM)

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
	cstyle -chpPv $(SRCS) $(SRCS_DMADM) $(HDRS)

clean:
	$(RM) $(DMADM) $(MODULE32) $(MODULE64) $(OBJS32) $(OBJS64)
