RENDER = ../../examples/render_bmp/render_bmp
INPUT = ../example_text.txt

justified.bmp: $(RENDER) $(INPUT)
	$(RENDER) DejaVuSerif16 $@ "`cat $(INPUT)`"