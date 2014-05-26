OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

%.o:%.c
	gcc -g -Wall -c $< -o $@
	
debug:$(OBJS)
	gcc -o luao $(OBJS)
	./luac -o test.lc test.lua
	./luac -o test_cf.lc test_cf.lua

release:$(OBJS)
	gcc -o luao $(OBJS)
	./luac -o test.lc test.lua
	./luac -o test_cf.lc test_cf.lua

clean:
	rm *.o
	rm luao
	rm test*.lc
