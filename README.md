# Battery Watcher

Lightweight C daemon that monitors battery level and sends notifications at critical thresholds.

## Features

- **Event-driven + polling hybrid**: Uses ACPI events for instant AC plug detection
- **Low resource usage**: Polls only when discharging, sleeps otherwise
- **Smart notifications**: Each threshold fires once per discharge cycle
- **No log spam**: Uses syslog/journal, respects LOG_NOTICE level
- Thresholds: 25%, 15%, 10%, 5%

## Build

```bash
make
```

## Install

```bash
sudo make install
```

Creates:
- `/usr/local/bin/battery-watcher`
- `/etc/systemd/system/battery-watcher.service`

## Configuration

Copy the example env file:

```bash
sudo cp .env.example /etc/battery-watcher.conf
```

Edit `/etc/battery-watcher.conf`:

```
BATTERY_NOTIFY_CMD=/path/to/your/notify-script
```

## Usage

```
battery-watcher [-i INTERVAL] [-d] [-h]

  -i SECONDS    Poll interval when discharging (default: 60)
  -d            Daemonize
  -h            Help
```

### Systemd

```bash
sudo systemctl enable --now battery-watcher
```

### Logs

```bash
journalctl -u battery-watcher -f
```

Logs are written to syslog (LOG_DAEMON facility). Important events use LOG_NOTICE, debug info uses LOG_DEBUG (hidden by default in journal).

To see debug logs:
```bash
journalctl -u battery-watcher -p debug -f
```

## How It Works

1. Connects to acpid socket for AC adapter events
2. When charging: sleeps, no polling
3. When unplugged: polls battery every 60s
4. On threshold crossing: sends notification once
5. Resets when plugged in

## Notifications

Uses `kc-notify` (ntfy.sh wrapper). Configure in `~/.openclaw/data/ntfy-profiles.yml`.

Override via `BATTERY_NOTIFY_CMD` environment variable.

## License

MIT
