SOURCE_FILES=src/median.c src/grid_gen.c src/result_set.c src/latlon_reader.c src/kd_tree.c
LDFLAGS=-lm -lproj
CFLAGS=-std=c99 -Wall
DFLAGS=-DREDUCE_MEDIAN

caspian: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) $(DFLAGS) -O3 -mtune=native -o bin/caspian

debug: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) $(DFLAGS) -DDEBUG -ggdb -o bin/debug

all: caspian

clean:
	rm -f bin/*
