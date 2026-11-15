#ifndef CONFIG_H
#define CONFIG_H

// Poll interval when discharging (can be longer since we use ACPI events)
#define DEFAULT_POLL_INTERVAL 60
// Runtime directory for the pid file. /run is only used when the environment
// gives no per-user runtime directory (system install).
#define DEFAULT_PID_FILE "/run/battery-watcher.pid"

// Name of the pid file inside the per-user runtime directory.
#define PID_FILE_NAME "battery-watcher.pid"

// ACPID socket for AC adapter events
#define ACPID_SOCKET "/run/acpid.socket"

// Battery sysfs path
#define BATTERY_PATH "/sys/class/power_supply/BAT0"

// Default notify command (override via BATTERY_NOTIFY_CMD env)
// Falls back to notify-send if not set
#define DEFAULT_NOTIFY_CMD "/usr/bin/notify-send"

// Notification thresholds (percent)
#define THRESHOLD_1 25
#define THRESHOLD_2 15
#define THRESHOLD_3 10
#define THRESHOLD_4 5

#define THRESHOLD_COUNT 4

#endif
