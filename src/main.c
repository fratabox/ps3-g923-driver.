#include <stdio.h>
#include <sysutil/sysutil.h>

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
    (void)argc;
    (void)argv;

    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_callback, NULL);
    printf("\nPS3 G923 - diagnostico inicial\n");
    printf("VID:PID esperado: 046D:C26D\n");
    printf("Comando PC confirmado: 0F 00 01 01 42 (OUT 0x01)\n");
    printf("Esta version solo valida el arranque PSL1GHT.\n");
    printf("La comunicacion USB/sys_usbd aun no esta implementada.\n");

    while (running)
        sysUtilCheckCallback();

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    return 0;
}
