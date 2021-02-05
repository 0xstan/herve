obj-m := herve.o
herve-objs := entry.o vmxasm.o vmx.o vcpu.o vmm.o utils.o exit.o vmcs.o ept.o
MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}
EXTRA_CFLAGS := -I.

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules $(EXTRA_CFLAGS)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
