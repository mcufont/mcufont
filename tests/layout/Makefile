RENDER = ../../examples/render_bmp/render_bmp
INPUT = ../example_text.txt

TESTS = serif16_justified_500.bmp \
	serif16_left_500.bmp \
	serif16_right_500.bmp \
	serif16_center_500.bmp \
	sans12_justified_500.bmp \
	sans12bw_justified_500.bmp \
	sans12bw_justified_500_bwfont.bmp \
	fixed_7x14_left_600.bmp \
	fixed_5x8_left_400.bmp

all: $(TESTS) $(TESTS:=.difference) run_tests

clean:
	rm -f $(TESTS) $(TESTS:=.difference)

serif16_justified_500.bmp: OPTS = -f DejaVuSerif16 -w 500 -a j
serif16_left_500.bmp:      OPTS = -f DejaVuSerif16 -w 500 -a l
serif16_right_500.bmp:     OPTS = -f DejaVuSerif16 -w 500 -a r
serif16_center_500.bmp:    OPTS = -f DejaVuSerif16 -w 500 -a c
sans12_justified_500.bmp:  OPTS = -f DejaVuSans12 -w 400 -a j
sans12bw_justified_500.bmp:OPTS = -f DejaVuSans12bw -w 400 -a j
sans12bw_justified_500_bwfont.bmp: OPTS = -f DejaVuSans12bw_bwfont -w 400 -a j
fixed_7x14_left_600.bmp:   OPTS = -f fixed_7x14 -w 600 -a l
fixed_5x8_left_400.bmp:    OPTS = -f fixed_5x8 -w 400 -a l

%.bmp: $(RENDER) $(INPUT)
	$(RENDER) $(OPTS) -o $@ "`cat $(INPUT)`"

%.bmp.difference: %.bmp %.bmp.expected
	composite -compose difference $^ $@
	mogrify -threshold 1 $@

run_tests:
	@echo "Comparing the results to expected files.."
	@$(foreach test,$(TESTS),cmp $(test) $(test).expected &&) true

# Run this to copy the outputs as the new expected files.
update_expected: $(TESTS)
	@echo "Updating all the expected files.."
	@$(foreach test,$(TESTS),cp $(test) $(test).expected &&) true
	cp sans12bw_justified_500.bmp sans12bw_justified_500_bwfont.bmp
