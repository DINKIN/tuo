-include .config

CC ?= "gcc"
KERNEL_DIR=`$(SHELL) uname -r`
#CFLAGS += -Wall -I/lib/modules/$(KERNEL_DIR)/build/include -g
CFLAGS += -Wall -I/lib/modules/2.6.23.14-64.fc7/build/include -g
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
