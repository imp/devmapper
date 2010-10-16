#
# Copyrigh 2010 Grigale Ltd. All rights reserved.
# Use is subject to license terms.
#
.KEEP_STATE:

include ../../Makefile.defs

# Build targets
DMADM		= dmadm

INCLUDES	= -I../../include
LDFLAGS		=
LINTFLAGS	= $(INCLUDES) -errsecurity=extended -Nlevel

HDRS		= include/sys/dm.h include/sys/dm_impl.h
SRCS		= dmadm.c

all: $(DMADM)

$(DMADM):	$(SRCS)
	$(CC) $(MACH_64) $(CFLAGS) -o $@ $(SRCS)


lint:
	$(LINT.c) $(SRCS)

cstyle:
	cstyle -chpPv $(SRCS) $(HDRS)

clean:
	$(RM) $(DMADM)
