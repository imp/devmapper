#
# Copyright 2011 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#
.KEEP_STATE:

include ../Makefile.defs

# Build targets
MODULE		= dm
PLUGINS		= dm_debug
PLUGINS		+= dm_linear

CONFFILE	= $(MODULE).conf
KERNEL		= -D_KERNEL
MACH_64		+= -xmodel=kernel
INCLUDES	= -I../include $(EXTRA_INCLUDES)
CFLAGS		+= -v $(KERNEL)
LDFLAGS		=
LINTFLAGS	= $(KERNEL) $(INCLUDES) -errsecurity=extended -Nlevel

HDRS		= ../include/sys/dm.h
HDRS		+= ../include/sys/dm_impl.h
HDRS		+= ../include/sys/dm_ops.h

SRCS		= dm.c
PLUGINS_SRCS	= $(PLUGINS:%=plugins/%.c)

OBJS32		= $(SRCS:%.c=32/%.o)
OBJS64		= $(SRCS:%.c=64/%.o)

PLUGIN_OBJS32	= $(PLUGIN_SRCS:%.c=32/%.o)
PLUGIN_OBJS64	= $(PLUGIN_SRCS:%.c=64/%.o)

MODULE32	= $(MODULE:%=32/%)
MODULE64	= $(MODULE:%=64/%)

PLUGINS32	= $(PLUGINS:%=32/%)
PLUGINS64	= $(PLUGINS:%=64/%)

TARGET		= $(MODULE32) $(MODULE64)
TARGET		+= $(PLUGINS32) $(PLUGINS64)

all: $(TARGET)

32 64:
	mkdir $@

32/%.o: %.c
	$(COMPILE.c) $(MACH_32) -o $@ $<

32/%.o: plugins/%.c
	$(COMPILE.c) $(MACH_32) -o $@ $<

64/%.o: %.c
	$(COMPILE.c) $(MACH_64) -o $@ $<

64/%.o: plugins/%.c
	$(COMPILE.c) $(MACH_64) -o $@ $<

32/%:	32 32/%.o
	$(LD) -r -o $@ $(LDFLAGS) $<

64/%:	64 64/%.o
	$(LD) -r -o $@ $(LDFLAGS) $<

install_files: $(CONFFILE) $(MODULE32) $(MODULE64) 
	pfexec $(CP) $(CONFFILE) /usr/kernel/drv
	pfexec $(CP) $(MODULE32) /usr/kernel/drv
	pfexec $(CP) $(MODULE64) /usr/kernel/drv/amd64
	pfexec $(MKDIR) -p /usr/kernel/misc/dm/amd64
	pfexec $(CP) $(PLUGINS32) /usr/kernel/misc/dm
	pfexec $(CP) $(PLUGINS64) /usr/kernel/misc/dm/amd64

install: install_files
	pfexec add_drv -v -n $(MODULE)

devlink:
	echo "type=ddi_pseudo;name=dm\tdm\M0\tmapper/control" >> /etc/devlink.tab

lint:
	$(LINT.c) $(SRCS)

cstyle:
	cstyle -chpPv $(SRCS) $(HDRS)

clean:
	$(RM) $(TARGET) $(MODULE32) $(MODULE64) $(OBJS32) $(OBJS64)
