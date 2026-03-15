#ifndef CONFIG_H
#define CONFIG_H

// Default configuration
#define DEFAULT_POLL_INTERVAL 30
#define DEFAULT_PID_FILE "/run/battery-watcher.pid"
#define DEFAULT_LOG_FILE "/var/log/battery-watcher.log"
#define BATTERY_PATH "/sys/class/power_supply/BAT0"
#define NOTIFY_CMD "/home/broklein/.local/bin/kc-notify"

// Notification thresholds (percent)
#define THRESHOLD_1 25
#define THRESHOLD_2 15
#define THRESHOLD_3 10
#define THRESHOLD_4 5

#define THRESHOLD_COUNT 4

#endif // CONFIG_H
