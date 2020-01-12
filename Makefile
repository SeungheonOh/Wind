CFLAGS+= -Wall -Wextra -pedantic
LDADD+= -lX11 -lXext
LDFLAGS=
PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC ?= gcc

all: wind

wind: wind.o
	$(CC) $(LDFLAGS) -O3 -o $@ $+ $(LDADD)

install: all 
	install -Dm 755 wind $(DESTDIR)$(BINDIR)/wind

uninstall:
	rm $(BINDIR)/wind

clean:
	rm -f wind *.o
