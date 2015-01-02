CFLAGS   += -O2 -Wall -fPIC

all:	mcpconfig

clean:
	rm -f *.o
	rm -f mcpconfig

mcpconfig: config.c hid.c mcp22xx.h
	$(CC) $(CFLAGS) -o $@ config.c hid.c

