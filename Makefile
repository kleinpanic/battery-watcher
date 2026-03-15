CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
TARGET = battery-watcher
SRC = src/main.c

PREFIX ?= /usr/local
# User-space install (no sudo needed)
USER_PREFIX ?= $(HOME)/.local
USER_SYSTEMD ?= $(HOME)/.config/systemd/user

.PHONY: all clean install uninstall test install-user uninstall-user

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

# User-space install (no sudo)
install-user: $(TARGET)
	install -d -m 755 $(USER_PREFIX)/bin/
	install -m 755 $(TARGET) $(USER_PREFIX)/bin/$(TARGET)
	install -d -m 755 $(USER_SYSTEMD)/
	install -m 644 systemd/battery-watcher.user.service $(USER_SYSTEMD)/battery-watcher.service

uninstall-user:
	rm -f $(USER_PREFIX)/bin/$(TARGET)
	rm -f $(USER_SYSTEMD)/battery-watcher.service

test: $(TARGET)
	@echo "Running tests..."
	./tests/run.sh
