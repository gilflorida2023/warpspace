CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lX11 -lncurses
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
USER_BINDIR = $(HOME)/projects/bin
BINARY = warpspace
INSTALL_BINARY = warpspace

all: $(BINARY)

$(BINARY): warpspace.c
	$(CC) $(CFLAGS) warpspace.c -o $(BINARY) $(LDFLAGS)

install:
	@if [ "$$(id -u)" -eq 0 ]; then \
		echo "Installing as root to $(BINDIR)/$(INSTALL_BINARY)"; \
		install -d $(BINDIR); \
		install -m 755 $(BINARY) $(BINDIR)/$(INSTALL_BINARY); \
	else \
		echo "Installing as user to $(USER_BINDIR)/$(INSTALL_BINARY)"; \
		install -d $(USER_BINDIR); \
		install -m 755 $(BINARY) $(USER_BINDIR)/$(INSTALL_BINARY); \
	fi

clean:
	rm -f $(BINARY)

.PHONY: all install clean
