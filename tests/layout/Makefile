RENDER = ../../examples/render_bmp/render_bmp
INPUT = ../example_text.txt

justified.bmp: $(RENDER) $(INPUT)
	$(RENDER) -f DejaVuSerif16 -w 400 -o $@ "`cat $(INPUT)`"