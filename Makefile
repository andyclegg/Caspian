SOURCE_FILES=src/median.c src/caspian.c src/result_set.c src/latlon_reader.c src/kd_tree.c src/data_handling.c src/reduction_functions.c src/grid.c src/gridding.c src/projector.c
LDFLAGS=-lm -lproj
CFLAGS=-std=c99 -Wall

all: caspian debug docs projcalc quickview

caspian: bin/caspian
projcalc: bin/projcalc
debug: bin/caspian-debug
docs: doc/caspian.pdf
quickview: bin/quickview

bin/projcalc: src/projection_calculator.c
	gcc $(CFLAGS) $(LDFLAGS) $? -o bin/projcalc

bin/caspian: $(SOURCE_FILES)
	gcc -fopenmp $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) -O3 -mtune=native -o bin/caspian

bin/caspian-debug: $(SOURCE_FILES)
	gcc -fopenmp $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) -DDEBUG -ggdb -o bin/caspian-debug

doc/caspian.pdf: src/doc/caspian.tex
	latexmk -cd $? -pdfdvi
	cp -f src/doc/caspian.pdf doc/caspian.pdf

bin/quickview: src/quickview
	cp src/quickview bin/quickview

.PHONY: clean
clean:
	rm -rf bin/* doc/*
	latexmk src/doc/caspian.tex -C -cd
