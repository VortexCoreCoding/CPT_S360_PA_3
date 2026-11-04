EXTRA_CFLAGS +=
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
SUBDIR := $(CURDIR)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules app

obj-m:= kmlab.o

modules:
	$(MAKE) -C $(KERNEL_SRC) M="$(SUBDIR)" modules

app: userapp.c userapp.h
	$(GCC) -o userapp userapp.c

clean:
	$(RM) -f userapp *~ *.cmd .*.cmd *.ko *.o *.mod *.mod.c Module.symvers modules.order
