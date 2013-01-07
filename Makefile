TESTS=df_main main
all: $(TESTS)

WSTREAM_LIB=/home/li/tstar/open-stream/libworkstream_df/
CFLAGS = -Wall -g -std=c99 -O2 -ffast-math
LDFLAGS = -L$(WSTREAM_LIB) -lm -lc -rdynamic -Wl,-rpath,$(WSTREAM_LIB) -lwstream_df
df_main: df_main.c tsu.h
	gcc $(CFLAGS) $(LDFLAGS) $^ -o $@

main: main.c
	gcc $(CFLAGS) $^ -o $@

run: main df_main
	./run.sh