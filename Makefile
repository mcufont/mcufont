all:
	make -C encoder
	make -C fonts
	make -C examples

clean:
	make -C encoder clean
	make -C fonts clean
	make -C examples clean
