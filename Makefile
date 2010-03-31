.PHONY: all clean

all:
	$(MAKE) -C libxdif
	$(MAKE) -C test

clean:
	$(MAKE) -C libxdif clean
	$(MAKE) -C test clean

