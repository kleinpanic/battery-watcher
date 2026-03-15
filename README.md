# Battery Watcher

Lightweight C daemon that monitors battery level and sends ntfy notifications at critical thresholds.

## Features

- Polls battery capacity via sysfs
- Only notifies when **discharging** (no spam while charging)
- Threshold notifications: 25%, 15%, 10%, 5%
- Each threshold fires once per discharge cycle
- Low resource usage: sleeps 99.9% of the time
- Systemd integration

## Build

```bash
make
```

## Install

```bash
sudo make install
```

This installs:
- Binary to `/usr/local/bin/battery-watcher`
- Systemd service to `/etc/systemd/system/battery-watcher.service`

## Usage

```
battery-watcher [-i INTERVAL] [-d] [-h]

  -i INTERVAL   Poll interval in seconds (default: 30)
  -d            Daemonize (run in background)
  -h            Show help
```

### Systemd Service

```bash
sudo systemctl enable --now battery-watcher
```

### Logs

```bash
journalctl -u battery-watcher -f
```

## Configuration

Edit `include/config.h` to change:
- Poll interval
- Threshold percentages
- Notification command
- Battery path

Rebuild after changes.

## Notifications

Uses `~/.local/bin/kc-notify` which wraps ntfy.sh. Configure profiles in `~/.openclaw/data/ntfy-profiles.yml`.

## License

MIT
