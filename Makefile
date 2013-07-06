all:
	make -C encoder
	make -C fonts
	make -C examples
	make -C tests

clean:
	make -C encoder clean
	make -C fonts clean
	make -C examples clean
	make -C tests clean
