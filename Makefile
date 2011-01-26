SOURCE_FILES=src/median.c src/grid_gen.c src/result_set.c src/latlon_reader.c src/kd_tree.c
LDFLAGS=-lm -lproj
CFLAGS=-std=c99 -Wall

all: caspian debug docs projcalc quickview

projcalc: src/projection_calculator.c
	gcc $(CFLAGS) $(LDFLAGS) $? -o bin/projcalc

caspian: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) -O3 -mtune=native -o bin/caspian

debug: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) -DDEBUG -ggdb -o bin/caspian-debug

docs: src/doc/caspian.tex
	latexmk -cd $? -pdfdvi
	cp -f src/doc/caspian.pdf doc/caspian.pdf

quickview: src/quickview
	cp src/quickview bin/quickview

clean:
	rm -rf bin/* doc/*
	latexmk -c src/doc/caspian.tex
