all:
	./build.sh

module:
	./build.sh

tools:
	$(MAKE) -C tools

clean:
	rm -rf build bin int3drv.ko
	$(MAKE) -C tools clean

load: all
	sudo insmod int3drv.ko

unload:
	sudo rmmod int3drv || true

test: all
	./test.sh

.PHONY: all module tools clean load unload test
