OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

%.o:%.c
	gcc -g -Wall -c $< -o $@
	
debug:$(OBJS)
	gcc -o luao $(OBJS) -lm
	./luac -o test.lc test.lua
	./luac -o test_cf.lc test_cf.lua
	./luac -o test_cp.lc test_cp.lua

release:$(OBJS)
	gcc -o luao $(OBJS) -lm

clean:
	rm *.o
	rm luao
	rm test*.lc
