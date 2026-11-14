#include <stdio.h>
#include <stdint.h>
#include <sysmodule/sysmodule.h>
#include <usb/usb.h>

#include "g923_protocol.h"
#include "g923_usb.h"

#define G923_MAX_ENDPOINTS 16

static const char g923_ldd_name[] = "g923";
static usbLddOps g923_ops = {
    g923_ldd_name,
    NULL,
    NULL,
    NULL
};

static s32 g923_device = -1;
static s32 g923_control_pipe = -1;
static s32 g923_endpoint_pipes[G923_MAX_ENDPOINTS];
static unsigned int g923_endpoint_count;
static int g923_usb_module_owned;
static int g923_usb_initialized;
static int g923_driver_registered;

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
    log_line("G923 detached; device id=%d\n", dev_id);
    return USB_DETACH_SUCCEEDED;
}

static void g923_on_configuration(s32 result, s32 count, void *arg)
{
    usbEndpointDescriptor *ep = NULL;
    unsigned int found = 0;
    (void)count;
    (void)arg;

    if (result != USB_HC_CC_NOERR) {
        log_line("SET_CONFIGURATION callback error=%d\n", result);
        return;
    }

    log_line("G923 configuration active; endpoint scan result=%d\n", result);

    while ((ep = (usbEndpointDescriptor *)prx_pointer(
                usbScanStaticDescriptor(g923_device, ep,
                                        USB_DESCRIPTOR_TYPE_ENDPOINT))) != NULL) {
        s32 pipe = usbOpenPipe(g923_device, ep);
        unsigned int type = ep->bmAttributes & USB_ENDPOINT_TRANSFER_TYPE_BITS;

        if (found < G923_MAX_ENDPOINTS && pipe >= 0)
            g923_endpoint_pipes[found++] = pipe;

        {
            FILE *f = fopen("/dev_hdd0/tmp/G923Diag.log", "a");
            if (f) {
                fprintf(f,
                        "Endpoint addr=0x%02X type=%u max_packet=%u interval=%u pipe=%d\n",
                        ep->bEndpointAddress, type, ep->wMaxPacketSize,
                        ep->bInterval, pipe);
                fclose(f);
            }
        }
        printf("G923 endpoint 0x%02X type=%u max=%u interval=%u pipe=%d\n",
               ep->bEndpointAddress, type, ep->wMaxPacketSize,
               ep->bInterval, pipe);
    }

    g923_endpoint_count = found;
    log_line("Usable endpoint pipes opened=%d\n", (int)found);
}

static s32 g923_attach(s32 dev_id)
{
    usbConfigDescriptor *config;
    s32 result;

    if (g923_device >= 0)
        return USB_ATTACH_FAILED;

    g923_device = dev_id;
    g923_control_pipe = usbOpenPipe(dev_id, NULL);
    log_line("G923 VID/PID matched; device id=%d\n", dev_id);
    log_line("Control pipe handle=%d\n", g923_control_pipe);

    config = (usbConfigDescriptor *)prx_pointer(
        usbScanStaticDescriptor(dev_id, NULL, USB_DESCRIPTOR_TYPE_CONFIG));

    if (config == NULL || g923_control_pipe < 0) {
        log_line("Unable to read configuration or open control pipe=%d\n", -1);
        if (g923_control_pipe >= 0)
            usbClosePipe(g923_control_pipe);
        g923_control_pipe = -1;
        g923_device = -1;
        return USB_ATTACH_FAILED;
    }

    result = usbSetConfiguration(g923_control_pipe,
                                 config->bConfigurationValue,
                                 g923_on_configuration, NULL);
    if (result != USB_OK) {
        log_line("SET_CONFIGURATION submission failed=%d\n", result);
        usbClosePipe(g923_control_pipe);
        g923_control_pipe = -1;
        g923_device = -1;
        return USB_ATTACH_FAILED;
    }

    log_line("SET_CONFIGURATION submitted; return=%d\n", result);
    return USB_ATTACH_SUCCEEDED;
}

int g923_usb_start(void)
{
    s32 result;
    unsigned int i;

    for (i = 0; i < G923_MAX_ENDPOINTS; ++i)
        g923_endpoint_pipes[i] = -1;

    {
        FILE *f = fopen("/dev_hdd0/tmp/G923Diag.log", "w");
        if (f) {
            fputs("Logitech G923 USB diagnostic start\n", f);
            fputs("Target VID:PID = 046D:C26D\n", f);
            fclose(f);
        }
    }

    result = sysModuleLoad(SYSMODULE_USB);
    if (result != 0 && result != SYSMODULE_ERR_DUPLICATE) {
        log_line("sysModuleLoad(USB) failed=%d\n", result);
        return result;
    }
    if (result == 0)
        g923_usb_module_owned = 1;
    log_line("USB module load result=%d\n", result);

    result = usbInit();
    if (result != USB_OK && result != USB_ERR_ALREADY_INITIALIZED) {
        log_line("usbInit failed=%d\n", result);
        return result;
    }
    if (result == USB_OK)
        g923_usb_initialized = 1;
    log_line("usbInit result=%d\n", result);

    g923_ops.probe = g923_probe;
    g923_ops.attach = g923_attach;
    g923_ops.detach = g923_detach;

    result = usbRegisterExtraLdd(&g923_ops,
                                 G923_USB_VENDOR_ID,
                                 G923_USB_PRODUCT_ID);
    log_line("usbRegisterExtraLdd result=%d\n", result);
    if (result == USB_OK)
        g923_driver_registered = 1;

    return result;
}

void g923_usb_stop(void)
{
    if (g923_device >= 0)
        g923_detach(g923_device);

    if (g923_driver_registered) {
        usbUnregisterExtraLdd(&g923_ops);
        g923_driver_registered = 0;
    }
    if (g923_usb_initialized) {
        usbEnd();
        g923_usb_initialized = 0;
    }
    if (g923_usb_module_owned) {
        sysModuleUnload(SYSMODULE_USB);
        g923_usb_module_owned = 0;
    }
}
