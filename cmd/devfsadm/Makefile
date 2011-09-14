#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#
.KEEP_STATE:

include ../../Makefile.defs

EXTRA_INCLUDES	= -I$(ON_SRC)/usr/src/cmd/devfsadm

# Build targets
DM_PLUGIN	= GRIGALE_dm_link.so

INCLUDES	= -I../../include $(EXTRA_INCLUDES)
LDFLAGS		=
LINTFLAGS	= $(INCLUDES) -errsecurity=extended -Nlevel

HDRS		= include/sys/dm.h include/sys/dm_impl.h
SRCS		= dm_link.c

all: $(DM_PLUGIN)

$(DM_PLUGIN):	$(OBJS)
	$(CC) -G $(MACH) $(CFLAGS) -o $@ $(SRCS)


lint:
	$(LINT.c) $(SRCS)

cstyle:
	cstyle -chpPv $(SRCS) $(HDRS)

clean:
	$(RM) $(DMADM)
