#include <libusb-1.0/libusb.h>
#include <sensors/sensors.h>

#define I_VENDOR_ID 0x1c57
// Screen's receiving endpoint
#define I_OUT_ENDPOINT 0x03
// 1M to wait 1 sec
#define I_SLEEP_INTERVAL 1000000
#define I_USB_TIMEOUT 100
#define I_USB_INTERFACE 0
// Package size must be 64 bytes
#define I_PAYLOAD_SIZE 64
// Each package must begin with this
#define I_CMD_BYTE 0x02
// For idle request
#define I_IDLE_BM_REQUEST_TYPE 0x21
#define I_IDLE_REQUEST 0x0A

#define ERR_LUSB_INIT -1
#define ERR_LUSB_HANDLE -2
#define ERR_LUSB_DEV_DETACH -3
#define ERR_LUSB_CLAIM -4
#define ERR_SENS_INIT -5
#define ERR_SENS_NOT_FOUND -6

// Log info to stdout
static void info(char *text);
// Log error to stderr
static void err(char *text);

// Create new USB context
static int init_usb_ctx(libusb_context **ctx);

// Create new USB handler
static int usb_device_open(libusb_context *ctx, libusb_device_handle **handle,
                           int should_test_additional_hw);

// Detach and claim USB device interface
static int init_usb_interface(libusb_context *ctx, libusb_device_handle *handle,
                              int ifnum);

// Release specified usb device interface and attach it to kernel
static void release_usb_interface(libusb_device_handle *handle, int ifnum);

// Find appropriate temperature sensor
static int find_temp_sensor(sensors_chip_name **found_chip, int *num,
                            char *sensor_label);

// Send temperature value to screen
static int send_temp_over_usb(libusb_device_handle *handle,
                              sensors_chip_name *chip, int subfeat_num,
                              int *transferred);

// Handler for SIGINT
static void handle_sigint(int _);
