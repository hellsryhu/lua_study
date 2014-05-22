OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

%.o:%.c
	gcc -g -Wall -c $< -o $@
	
debug:$(OBJS)
	gcc -o chunk_spy $(OBJS)
	./luac -o test.lc test.lua
	./luac -o test_cf.lc test_cf.lua

release:$(OBJS)
	gcc -o chunk_spy $(OBJS)
	./luac -o test.lc test.lua
	./luac -o test_cf.lc test_cf.lua

clean:
	rm *.o
	rm chunk_spy
	rm test*.lc
