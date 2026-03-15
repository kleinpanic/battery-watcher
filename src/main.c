#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include "config.h"

static volatile int running = 1;
static int poll_interval = DEFAULT_POLL_INTERVAL;
static int daemonize = 0;

// Threshold state tracking
typedef struct {
    int threshold;
    int notified;  // 0 = not notified this discharge cycle
} threshold_state;

static threshold_state thresholds[THRESHOLD_COUNT] = {
    {THRESHOLD_1, 0},
    {THRESHOLD_2, 0},
    {THRESHOLD_3, 0},
    {THRESHOLD_4, 0}
};

void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            running = 0;
            break;
    }
}

int read_battery_capacity(void) {
    FILE *fp = fopen(BATTERY_PATH "/capacity", "r");
    if (!fp) {
        syslog(LOG_ERR, "Failed to open capacity file: %s", strerror(errno));
        return -1;
    }
    
    int capacity;
    if (fscanf(fp, "%d", &capacity) != 1) {
        syslog(LOG_ERR, "Failed to read capacity");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return capacity;
}

int read_battery_status(char *status, size_t len) {
    FILE *fp = fopen(BATTERY_PATH "/status", "r");
    if (!fp) {
        syslog(LOG_ERR, "Failed to open status file: %s", strerror(errno));
        return -1;
    }
    
    if (!fgets(status, len, fp)) {
        syslog(LOG_ERR, "Failed to read status");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // Strip newline
    status[strcspn(status, "\n")] = 0;
    return 0;
}

int send_notification(int capacity) {
    char cmd[512];
    char msg[128];
    
    snprintf(msg, sizeof(msg), "Battery at %d%%", capacity);
    snprintf(cmd, sizeof(cmd), "%s alert \"%s\"", NOTIFY_CMD, msg);
    
    int ret = system(cmd);
    if (ret != 0) {
        syslog(LOG_WARNING, "Notification command failed with status %d", ret);
        return -1;
    }
    
    syslog(LOG_INFO, "Sent notification: %s", msg);
    return 0;
}

void check_thresholds(int capacity, int is_discharging) {
    for (int i = 0; i < THRESHOLD_COUNT; i++) {
        if (capacity <= thresholds[i].threshold) {
            if (is_discharging && !thresholds[i].notified) {
                // Crossing threshold while discharging - notify
                send_notification(capacity);
                thresholds[i].notified = 1;
            }
        } else {
            // Capacity rose above threshold - reset notification flag
            thresholds[i].notified = 0;
        }
    }
}

void reset_thresholds(void) {
    for (int i = 0; i < THRESHOLD_COUNT; i++) {
        thresholds[i].notified = 0;
    }
}

void write_pid_file(void) {
    FILE *fp = fopen(DEFAULT_PID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

void daemon_start(void) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
        exit(1);
    }
    if (pid > 0) {
        exit(0);  // Parent exits
    }
    
    // Child continues
    if (setsid() < 0) {
        fprintf(stderr, "Failed to setsid: %s\n", strerror(errno));
        exit(1);
    }
    
    // Redirect standard files
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    
    umask(0);
    chdir("/");
}

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Battery watcher daemon - monitors battery and sends ntfy notifications.\n\n");
    printf("Options:\n");
    printf("  -i INTERVAL   Poll interval in seconds (default: %d)\n", DEFAULT_POLL_INTERVAL);
    printf("  -d            Daemonize (run in background)\n");
    printf("  -h            Show this help\n");
    printf("\nThresholds: %d%%, %d%%, %d%%, %d%%\n", THRESHOLD_1, THRESHOLD_2, THRESHOLD_3, THRESHOLD_4);
    printf("Notifications sent via: %s\n", NOTIFY_CMD);
}

int main(int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, "i:dh")) != -1) {
        switch (opt) {
            case 'i':
                poll_interval = atoi(optarg);
                if (poll_interval < 5) {
                    fprintf(stderr, "Minimum poll interval is 5 seconds\n");
                    exit(1);
                }
                break;
            case 'd':
                daemonize = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    // Setup signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, SIG_IGN);
    
    // Open syslog
    openlog("battery-watcher", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "Starting battery watcher (interval: %ds)", poll_interval);
    
    if (daemonize) {
        daemon_start();
    }
    
    write_pid_file();
    
    char status[32];
    int last_capacity = -1;
    int was_charging = 0;
    
    while (running) {
        int capacity = read_battery_capacity();
        
        if (read_battery_status(status, sizeof(status)) == 0) {
            int is_discharging = (strcmp(status, "Discharging") == 0);
            int is_charging = (strcmp(status, "Charging") == 0);
            
            // Reset thresholds when charging starts
            if (is_charging && !was_charging) {
                syslog(LOG_INFO, "Charging started - resetting threshold notifications");
                reset_thresholds();
            }
            was_charging = is_charging;
            
            if (capacity >= 0 && is_discharging) {
                check_thresholds(capacity, 1);
            }
            
            // Log state change
            if (capacity != last_capacity) {
                syslog(LOG_DEBUG, "Battery: %d%% (%s)", capacity, status);
                last_capacity = capacity;
            }
        }
        
        sleep(poll_interval);
    }
    
    // Cleanup
    unlink(DEFAULT_PID_FILE);
    syslog(LOG_INFO, "Battery watcher stopped");
    closelog();
    
    return 0;
}
