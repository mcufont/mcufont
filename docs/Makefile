all: index.html decoder.html encoder.html rlefont.html

%.png: %.svg
	rsvg $< $@

%.html: %.rst
	rst2html --stylesheet=lsr.css --link-stylesheet $< $@
	sed -i 's!</head>!<link href="favicon.ico" type="image/x-icon" rel="shortcut icon" />\n</head>!' $@
