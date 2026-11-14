#include <stdio.h>
#include <sysutil/sysutil.h>

#include "g923_usb.h"

static volatile int running = 1;

static void sysutil_callback(uint64_t status, uint64_t param, void *userdata)
{
    (void)param;
    (void)userdata;
    if (status == SYSUTIL_EXIT_GAME)
        running = 0;
}

int main(int argc, char **argv)
{
    int usb_result;

    (void)argc;
    (void)argv;

    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_callback, NULL);

    printf("\nLogitech G923 PS3 USB diagnostic\n");
    printf("Expected device: VID:PID 046D:C26D\n");
    printf("Starting PSL1GHT USB host library...\n");

    usb_result = g923_usb_start();
    printf("USB diagnostic start result: %d\n", usb_result);
    printf("Detailed log: /dev_hdd0/tmp/G923Diag.log\n");
    printf("This build scans descriptors only; it does not send force feedback.\n");

    while (running)
        sysUtilCheckCallback();

    g923_usb_stop();
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    return 0;
}
