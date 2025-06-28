#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) {
    struct rtcdate d;
    if (date(&d) == -1) {
        exit();
    }
    printf(1, "%d-%d-%dT%d:%d:%d\n", d.year, d.month, d.day, d.hour, d.minute, d.second);
    return 0;
}
