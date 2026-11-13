/* Regression harness for check_thresholds(): a single battery reading that
 * sits below several thresholds must produce exactly one notification.
 *
 * src/main.c is compiled with -Dmain=bw_main by tests/run.sh so this file
 * can own main(). tests/run.sh counts the notification lines on stdout.
 */
#include <stdlib.h>
#include "config.h"

void check_thresholds(int capacity, int is_discharging);

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

    return 0;
}
