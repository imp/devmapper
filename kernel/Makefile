#
# Copyrigh 2011 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#
.KEEP_STATE:

include ../Makefile.defs

# Build targets
MODULE		= dm

CONFFILE	= $(MODULE).conf
KERNEL		= -D_KERNEL
MACH_64		+= -xmodel=kernel
INCLUDES	= -I../include $(EXTRA_INCLUDES)
CFLAGS		+= -v $(KERNEL)
LDFLAGS		=
LINTFLAGS	= $(KERNEL) $(INCLUDES) -errsecurity=extended -Nlevel

HDRS		= ../include/sys/dm.h ../include/sys/dm_impl.h
SRCS		= dm.c

OBJS32		= $(SRCS:%.c=32/%.o)
OBJS64		= $(SRCS:%.c=64/%.o)

MODULE32	= 32/$(MODULE)
MODULE64	= 64/$(MODULE)

all: $(MODULE32) $(MODULE64)

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
	cstyle -chpPv $(SRCS) $(HDRS)

clean:
	$(RM) $(MODULE32) $(MODULE64) $(OBJS32) $(OBJS64)
