#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include "config.h"

static volatile int running = 1;
static int poll_interval = DEFAULT_POLL_INTERVAL;
static int daemonize = 0;

// Threshold state tracking
typedef struct {
    int threshold;
    int notified;
} threshold_state;

static threshold_state thresholds[THRESHOLD_COUNT] = {
    {THRESHOLD_1, 0},
    {THRESHOLD_2, 0},
    {THRESHOLD_3, 0},
    {THRESHOLD_4, 0}
};

// Get notify command from env or default
static const char* get_notify_cmd(void) {
    const char *cmd = getenv("BATTERY_NOTIFY_CMD");
    return cmd ? cmd : DEFAULT_NOTIFY_CMD;
}

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
    if (!fp) return -1;
    
    int capacity;
    if (fscanf(fp, "%d", &capacity) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return capacity;
}

int read_battery_status(char *status, size_t len) {
    FILE *fp = fopen(BATTERY_PATH "/status", "r");
    if (!fp) return -1;
    
    if (!fgets(status, len, fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    status[strcspn(status, "\n")] = 0;
    return 0;
}

int send_notification(int capacity) {
    char cmd[512];
    char msg[64];
    const char *notify_cmd = get_notify_cmd();
    
    snprintf(msg, sizeof(msg), "Battery at %d%%", capacity);
    snprintf(cmd, sizeof(cmd), "%s \"%s\"", notify_cmd, msg);
    
    int ret = system(cmd);
    if (ret != 0) {
        syslog(LOG_WARNING, "Notification failed: %s", msg);
        return -1;
    }
    
    syslog(LOG_NOTICE, "Sent: %s", msg);
    return 0;
}

void check_thresholds(int capacity, int is_discharging) {
    for (int i = 0; i < THRESHOLD_COUNT; i++) {
        if (capacity <= thresholds[i].threshold) {
            if (is_discharging && !thresholds[i].notified) {
                send_notification(capacity);
                thresholds[i].notified = 1;
            }
        } else {
            thresholds[i].notified = 0;
        }
    }
}

void reset_thresholds(void) {
    for (int i = 0; i < THRESHOLD_COUNT; i++) {
        thresholds[i].notified = 0;
    }
}

// Connect to acpid socket for AC events
int connect_acpid(void) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ACPID_SOCKET, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

// Parse ACPI event for AC adapter change
// Returns: 1 if unplugged, -1 if plugged, 0 if other
int parse_acpi_event(const char *event) {
    // acpid events look like: "ac_adapter ACPI0003:00 00000080 00000000"
    // or: "battery BAT0 00000080 00000001" (capacity change, but unreliable)
    
    if (strstr(event, "ac_adapter")) {
        if (strstr(event, "00000000")) {
            return -1; // Plugged in
        } else if (strstr(event, "00000001")) {
            return 1;  // Unplugged
        }
    }
    return 0;
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
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    
    if (setsid() < 0) exit(1);
    
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    
    umask(0);
    chdir("/");
}

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Battery watcher - monitors battery and sends notifications.\n\n");
    printf("Options:\n");
    printf("  -i SECONDS    Poll interval when discharging (default: %d)\n", DEFAULT_POLL_INTERVAL);
    printf("  -d            Daemonize\n");
    printf("  -h            Show help\n");
    printf("\nEnvironment:\n");
    printf("  BATTERY_NOTIFY_CMD  Full command prefix (default: %s \"Battery\")\n", DEFAULT_NOTIFY_CMD);
    printf("\nThresholds: %d%%, %d%%, %d%%, %d%%\n", THRESHOLD_1, THRESHOLD_2, THRESHOLD_3, THRESHOLD_4);
}

int main(int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, "i:dh")) != -1) {
        switch (opt) {
            case 'i':
                poll_interval = atoi(optarg);
                if (poll_interval < 10) poll_interval = 10;
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
    
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, SIG_IGN);
    
    // Use LOG_DAEMON facility - logs to /var/log/daemon.log or journal
    // LOG_NOTICE and above are important, LOG_DEBUG/INFO are filtered by default
    openlog("battery-watcher", LOG_PID, LOG_DAEMON);
    
    if (daemonize) {
        daemon_start();
    }
    
    write_pid_file();
    
    int acpi_sock = connect_acpid();
    if (acpi_sock >= 0) {
        syslog(LOG_INFO, "Connected to acpid socket");
    } else {
        syslog(LOG_INFO, "No acpid, using polling only (interval: %ds)", poll_interval);
    }
    
    char status[32];
    int was_charging = 0;
    time_t last_check = 0;
    
    while (running) {
        time_t now = time(NULL);
        
        // Check battery state
        if (read_battery_status(status, sizeof(status)) == 0) {
            int is_discharging = (strcmp(status, "Discharging") == 0);
            int is_charging = (strcmp(status, "Charging") == 0);
            
            // Reset on charging
            if (is_charging && !was_charging) {
                syslog(LOG_INFO, "Charging started - thresholds reset");
                reset_thresholds();
            }
            was_charging = is_charging;
            
            // Only poll capacity when discharging and enough time passed
            if (is_discharging && (now - last_check >= poll_interval)) {
                int capacity = read_battery_capacity();
                if (capacity >= 0) {
                    check_thresholds(capacity, 1);
                }
                last_check = now;
            }
        }
        
        // Use select to wait for ACPI events or timeout
        if (acpi_sock >= 0) {
            fd_set fds;
            struct timeval tv;
            
            FD_ZERO(&fds);
            FD_SET(acpi_sock, &fds);
            
            // Wait up to poll_interval for ACPI event
            tv.tv_sec = poll_interval;
            tv.tv_usec = 0;
            
            int ret = select(acpi_sock + 1, &fds, NULL, NULL, &tv);
            
            if (ret > 0 && FD_ISSET(acpi_sock, &fds)) {
                char buf[256];
                ssize_t n = read(acpi_sock, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = 0;
                    int event = parse_acpi_event(buf);
                    if (event != 0) {
                        syslog(LOG_DEBUG, "ACPI: %s", buf);
                        // Force immediate check on AC event
                        last_check = 0;
                    }
                } else if (n == 0) {
                    // Socket closed, reconnect
                    close(acpi_sock);
                    acpi_sock = connect_acpid();
                    if (acpi_sock < 0) {
                        syslog(LOG_WARNING, "Lost acpid connection");
                    }
                }
            }
        } else {
            // No acpid, just sleep
            sleep(poll_interval);
        }
    }
    
    if (acpi_sock >= 0) close(acpi_sock);
    unlink(DEFAULT_PID_FILE);
    syslog(LOG_INFO, "Stopped");
    closelog();
    
    return 0;
}
