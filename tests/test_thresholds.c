/* Regression harness for check_thresholds(): a single battery reading that
 * sits below several thresholds must produce exactly one notification.
 *
 * src/main.c is compiled with -Dmain=bw_main by tests/run.sh so this file
 * can own main(). tests/run.sh counts the notification lines on stdout.
 */
#include <stdio.h>
#include <stdlib.h>
#include "config.h"

void check_thresholds(int capacity, int is_discharging);
int parse_acpi_event(const char *event);

static int failures;

static void expect_eq(int actual, int expected, const char *what)
{
    if (actual != expected) {
        printf("%s: got %d, expected %d\n", what, actual, expected);
        failures++;
    }
}

int main(void)
{
    /* Each notification prints exactly one line through /bin/echo. */
    setenv("BATTERY_NOTIFY_CMD", "/bin/echo", 1);

    /* 30% is above every threshold, so it clears the notified flags. */
    check_thresholds(30, 1);

    /* 8% is below all four thresholds in one reading: one notice. */
    check_thresholds(8, 1);

    /* A repeated reading at the same level adds no notice. */
    check_thresholds(8, 1);

    /*
     * The trailing value of an ac_adapter event is the AC presence state, the
     * same value sysfs exposes as AC/online: 1 when the adapter is present,
     * 0 when it is removed.
     */
    expect_eq(parse_acpi_event("ac_adapter ACPI0003:00 00000080 00000001"), -1,
              "adapter present");
    expect_eq(parse_acpi_event("ac_adapter ACPI0003:00 00000080 00000000"), 1,
              "adapter removed");
    expect_eq(parse_acpi_event("battery BAT0 00000080 00000001"), 0,
              "non-AC event ignored");

    return failures ? 1 : 0;
}
