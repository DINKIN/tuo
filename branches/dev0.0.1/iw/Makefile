-include .config

CC ?= "gcc"
KERNEL_VERSION=2.6.24-16-generic
CFLAGS += -Wall -I/lib/modules/$(KERNEL_VERSION)/build/include -g
LDFLAGS += -lnl

OBJS = iw.o interface.o info.o station.o util.o mpath.o
ALL = iw

all: verify_config $(ALL)

iw:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o iw $(LDFLAGS)

clean:
	rm -f iw *.o *~

verify_config:
	@if [ ! -r .config ]; then \
		echo 'Building iw requires a configuration file'; \
		echo '(.config). cp defconfig .config and edit.'; \
		exit 1; \
	fi
