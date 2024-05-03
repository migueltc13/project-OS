TEX = xelatex
TEX_FLAGS = -output-directory=$(OUT) -shell-restricted
OUT = output

.PHONY: build install

build:
	# Create the output directory
	mkdir -p $(OUT)
	# Generate the index
	makeindex $(OUT)/main.nlo -s nomencl.ist -o $(OUT)/main.nls
	# Compile the document
	$(TEX) $(TEX_FLAGS) main.tex

install:
	# Install the required packages
	sudo apt install -y texlive-xetex texlive-lang-portuguese texlive-latex-recommended ttf-mscorefonts-installer fonts-stix
	# Check if Arial font is installed
	sudo fc-cache -f && fc-match Arial
