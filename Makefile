SOURCE_FILES=src/median.c src/caspian.c src/result_set.c src/rawfile_coordinate_reader.c src/kd_tree.c src/data_handling.c src/reduction_functions.c src/grid.c src/gridding.c src/proj_projector.c src/io_helper.c
OBJECTS=build/median.o build/caspian.o build/result_set.o build/rawfile_coordinate_reader.o build/kd_tree.o build/data_handling.o build/reduction_functions.o build/grid.o build/gridding.o build/proj_projector.o build/io_helper.o
CC=gcc
LDFLAGS=-lm -lproj
CFLAGS=-fopenmp -std=c99 -Wall -Werror
OPT_FLAGS=-O3 -mtune=native
DEBUG_FLAGS=-DDEBUG -ggdb
OPT_CC=$(CC) $(CFLAGS) $(OPT_FLAGS) -c

minimal: caspian quickview
all: caspian projcalc debug docs quickview

caspian: bin/caspian
projcalc: bin/projcalc
debug: bin/caspian-debug
docs: doc/caspian.pdf doc/html/index.html
quickview: bin/quickview
check: build_testcases run_testcases

bin/projcalc: src/projection_calculator.c
	$(CC) $(CFLAGS) $(LDFLAGS) $? -o bin/projcalc

build/median.o: src/median.c src/median.h
	$(OPT_CC) src/median.c -o build/median.o

build/caspian.o: src/caspian.c src/coordinate_reader.h src/data_handling.h src/gridding.h src/grid.h src/io_helper.h src/kd_tree.h src/proj_projector.h src/projector.h src/rawfile_coordinate_reader.h src/reduction_functions.h src/spatial_index.h
	$(OPT_CC) src/caspian.c -o build/caspian.o

build/result_set.o: src/result_set.c src/result_set.h
	$(OPT_CC) src/result_set.c -o build/result_set.o

build/rawfile_coordinate_reader.o: src/rawfile_coordinate_reader.c src/rawfile_coordinate_reader.h src/coordinate_reader.h
	$(OPT_CC) src/rawfile_coordinate_reader.c -o build/rawfile_coordinate_reader.o

build/kd_tree.o: src/kd_tree.c src/kd_tree.h src/coordinate_reader.h src/data_handling.h src/spatial_index.h src/proj_projector.h src/projector.h src/result_set.h
	$(OPT_CC) src/kd_tree.c -o build/kd_tree.o

build/data_handling.o: src/data_handling.c src/data_handling.h
	$(OPT_CC) src/data_handling.c -o build/data_handling.o

build/reduction_functions.o: src/reduction_functions.c src/reduction_functions.h src/median.h src/result_set.h
	$(OPT_CC) src/reduction_functions.c -o build/reduction_functions.o

build/grid.o: src/grid.c src/grid.h src/projector.h
	$(OPT_CC) src/grid.c -o build/grid.o

build/gridding.o: src/gridding.c src/gridding.h src/io_spec.h src/reduction_functions.h src/result_set.h
	$(OPT_CC) src/gridding.c -o build/gridding.o

build/proj_projector.o: src/proj_projector.c src/proj_projector.h src/projector.h
	$(OPT_CC) src/proj_projector.c -o build/proj_projector.o

build/io_helper.o: src/io_helper.c src/io_helper.h
	$(OPT_CC) src/io_helper.c -o build/io_helper.o

bin/caspian: $(OBJECTS)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(LDFLAGS) $(OBJECTS) -o bin/caspian

bin/caspian-debug: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) $(DEBUG_FLAGS) -o bin/caspian-debug

doc/caspian.pdf: src/doc/caspian.tex
	latexmk -cd $? -pdfdvi
	cp -f src/doc/caspian.pdf doc/caspian.pdf

doc/html/index.html: src/*
	doxygen src/Doxyfile

bin/quickview: src/quickview
	cp src/quickview bin/quickview

build_testcases: caspian test/check_data_handling.test test/check_rawfile_coordinate_reader.test test/check_grid.test test/check_io_helper.test test/check_median.test

test/check_data_handling.test: build/data_handling.o test/check_data_handling.c
	gcc test/check_data_handling.c build/data_handling.o -lcheck -o test/check_data_handling.test

test/check_rawfile_coordinate_reader.test: build/rawfile_coordinate_reader.o build/proj_projector.o test/check_rawfile_coordinate_reader.c
	gcc test/check_rawfile_coordinate_reader.c build/rawfile_coordinate_reader.o build/proj_projector.o -lcheck -lproj -o test/check_rawfile_coordinate_reader.test

test/check_grid.test: build/grid.o build/proj_projector.o test/check_grid.c
	gcc test/check_grid.c build/grid.o build/proj_projector.o -lcheck -lproj -o test/check_grid.test

test/check_io_helper.test: build/io_helper.o test/check_io_helper.c
	gcc test/check_io_helper.c build/io_helper.o -lcheck -o test/check_io_helper.test

test/check_median.test: build/median.o test/check_median.c
	gcc test/check_median.c build/median.o -lcheck -o test/check_median.test

run_testcases: build_testcases
	./test/check_data_handling.test
	./test/check_rawfile_coordinate_reader.test
	./test/check_grid.test
	./test/check_io_helper.test
	./test/check_median.test

.PHONY: clean
clean:
	rm -rf bin/* doc/* build/* test/*.test
	latexmk src/doc/caspian.tex -C -cd
