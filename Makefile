CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
TARGET = battery-watcher
SRC = src/main.c

PREFIX ?= /usr/local

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d -m 755 $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -d -m 755 $(DESTDIR)/etc/systemd/system/
	install -m 644 systemd/battery-watcher.service $(DESTDIR)/etc/systemd/system/
	install -m 644 .env.example $(DESTDIR)/etc/battery-watcher.conf.example

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)/etc/systemd/system/battery-watcher.service

test: $(TARGET)
	@echo "Running tests..."
	./tests/run.sh
