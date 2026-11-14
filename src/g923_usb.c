#include <stdio.h>
#include <stdint.h>
#include <ppu-asm.h>
#include <sysmodule/sysmodule.h>
#include <usb/usb.h>

#include "g923_protocol.h"
#include "g923_usb.h"

/*
 * libusb's LDD registration ABI keeps the operations structure after the
 * registration call. Keep this 32-bit PRX-facing structure in static storage.
 */
typedef struct g923_ldd_ops32 {
    u32 name;
    u32 probe;
    u32 attach;
    u32 detach;
} g923_ldd_ops32;

extern s32 usbRegisterExtraLddEx(g923_ldd_ops32 *ops, u16 vendor_id, u16 product_id);
extern s32 usbUnregisterExtraLddEx(g923_ldd_ops32 *ops);

static const char g923_ldd_name[] = "g923";
static g923_ldd_ops32 g923_ops;
static s32 g923_device = -1;
static s32 g923_control_pipe = -1;
static s32 g923_endpoint_pipes[16];
static unsigned int g923_endpoint_count;
static FILE *g923_log_file;

static void log_line(const char *message, int value)
{
    FILE *f = fopen("/dev_hdd0/tmp/G923Diag.log", "a");
    if (f) {
        fprintf(f, message, value);
        fclose(f);
    }
    printf(message, value);
}

static void *prx_pointer(void *p)
{
    return (void *)(uintptr_t)(u32)(uintptr_t)p;
}

static u32 function_opd(s32 (*fn)(s32))
{
    return (u32)__get_opd32(fn);
}

static s32 g923_probe(s32 dev_id)
{
    (void)dev_id;
    return USB_PROBE_SUCCEEDED;
}

static s32 g923_attach(s32 dev_id);

static s32 g923_detach(s32 dev_id)
{
    unsigned int i;

    if (dev_id != g923_device)
        return USB_DETACH_SUCCEEDED;

    for (i = 0; i < g923_endpoint_count; ++i) {
        if (g923_endpoint_pipes[i] >= 0)
            usbClosePipe(g923_endpoint_pipes[i]);
        g923_endpoint_pipes[i] = -1;
    }
    g923_endpoint_count = 0;

    if (g923_control_pipe >= 0)
        usbClosePipe(g923_control_pipe);
    g923_control_pipe = -1;
    g923_device = -1;
    log_line("G923 USB detach, device id=%d\n", dev_id);
    return USB_DETACH_SUCCEEDED;
}

static void g923_on_configuration(s32 result, s32 count, void *arg)
{
    usbEndpointDescriptor *ep = NULL;
    unsigned int found = 0;
    (void)count;
    (void)arg;

    if (result != USB_HC_CC_NOERR) {
        log_line("G923 set-configuration callback error=%d\n", result);
        return;
    }

    log_line("G923 USB configuration set; scanning endpoints. result=%d\n", result);

    while ((ep = (usbEndpointDescriptor *)prx_pointer(
                usbScanStaticDescriptor(g923_device, ep, USB_DESCRIPTOR_TYPE_ENDPOINT))) != NULL) {
        s32 pipe;
        unsigned int type = ep->bmAttributes & USB_ENDPOINT_TRANSFER_TYPE_BITS;

        pipe = usbOpenPipe(g923_device, ep);
        if (found < 16)
            g923_endpoint_pipes[found++] = pipe;

        {
            FILE *f = fopen("/dev_hdd0/tmp/G923Diag.log", "a");
            if (f) {
                fprintf(f, "Endpoint addr=0x%02X type=%u max_packet=%u interval=%u pipe=%d\n",
                        ep->bEndpointAddress, type, ep->wMaxPacketSize, ep->bInterval, pipe);
                fclose(f);
            }
        }
        printf("G923 endpoint 0x%02X type=%u max=%u interval=%u pipe=%d\n",
               ep->bEndpointAddress, type, ep->wMaxPacketSize, ep->bInterval, pipe);
    }

    g923_endpoint_count = found;
    log_line("G923 endpoint descriptors found=%d\n", (int)found);
}

static s32 g923_attach(s32 dev_id)
{
    usbConfigDescriptor *config;
    s32 result;

    if (g923_device >= 0)
        return USB_ATTACH_FAILED;

    g923_device = dev_id;
    g923_control_pipe = usbOpenPipe(dev_id, NULL);
    log_line("G923 attach; device id=%d\n", dev_id);
    log_line("G923 control pipe=%d\n", g923_control_pipe);

    config = (usbConfigDescriptor *)prx_pointer(
        usbScanStaticDescriptor(dev_id, NULL, USB_DESCRIPTOR_TYPE_CONFIG));

    if (config == NULL || g923_control_pipe < 0) {
        log_line("G923 could not get config/control pipe, error=%d\n", -1);
        g923_device = -1;
        return USB_ATTACH_FAILED;
    }

    result = usbSetConfiguration(g923_control_pipe, config->bConfigurationValue,
                                 g923_on_configuration, NULL);
    if (result != 0) {
        log_line("G923 set-configuration submit failed=%d\n", result);
        usbClosePipe(g923_control_pipe);
        g923_control_pipe = -1;
        g923_device = -1;
        return USB_ATTACH_FAILED;
    }

    log_line("G923 configuration request submitted=%d\n", result);
    return USB_ATTACH_SUCCEEDED;
}

int g923_usb_start(void)
{
    s32 result;

    g923_log_file = fopen("/dev_hdd0/tmp/G923Diag.log", "w");
    if (g923_log_file) {
        fputs("Logitech G923 USB diagnostic start\n", g923_log_file);
        fclose(g923_log_file);
        g923_log_file = NULL;
    }

    result = sysModuleLoad(SYSMODULE_USB);
    if (result != 0 && result != SYSMODULE_ERR_DUPLICATE) {
        log_line("sysModuleLoad(USB) failed=%d\n", result);
        return result;
    }
    log_line("USB module load status=%d\n", result);

    result = usbInit();
    if (result != 0) {
        log_line("usbInit failed=%d\n", result);
        return result;
    }
    log_line("usbInit status=%d\n", result);

    g923_ops.name = (u32)__get_addr32(g923_ldd_name);
    g923_ops.probe = function_opd(g923_probe);
    g923_ops.attach = function_opd(g923_attach);
    g923_ops.detach = function_opd(g923_detach);

    result = usbRegisterExtraLddEx(&g923_ops, G923_USB_VENDOR_ID, G923_USB_PRODUCT_ID);
    log_line("G923 VID/PID driver registration result=%d\n", result);
    return result;
}

void g923_usb_stop(void)
{
    unsigned int i;

    if (g923_device >= 0)
        g923_detach(g923_device);

    usbUnregisterExtraLddEx(&g923_ops);
    usbEnd();
    sysModuleUnload(SYSMODULE_USB);
    for (i = 0; i < 16; ++i)
        g923_endpoint_pipes[i] = -1;
    g923_endpoint_count = 0;
}
