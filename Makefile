KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(CURDIR)/build
SRC_DIR := $(CURDIR)

all:
	mkdir -p $(BUILD_DIR)
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) src=$(SRC_DIR) modules

clean:
	rm -rf $(BUILD_DIR)

install:
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) modules_install
	depmod

reinstall: all
	rmmod $(name) || true
	insmod $(BUILD_DIR)/$(name).ko
	dmesg | tail -n 5


.PHONY: all clean install reinstall help
