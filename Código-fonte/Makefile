CC = g++
CFLAGS = -Wall
INSTALL = `which install`

mcp3008hwspi: mcp3008hwspi.cpp
	$(CC) $(CFLAGS) mcp3008hwspi.cpp -o mcp3008hwspi

install: mcp3008hwspi
	$(INSTALL) ./mcp3008hwspi /usr/local/bin/mcp3008hwspi

clean:
	rm -f mcp3008hwspi
