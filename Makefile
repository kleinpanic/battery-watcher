CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
TARGET = battery-watcher
SRC = src/main.c

PREFIX ?= /usr/local

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)
	install -d -m 755 $(DESTDIR)/etc/
	install -m 644 systemd/battery-watcher.service $(DESTDIR)/etc/systemd/system/

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)/etc/systemd/system/battery-watcher.service

test: $(TARGET)
	@echo "Running basic test..."
	./$(TARGET) -h
	@echo "OK"
