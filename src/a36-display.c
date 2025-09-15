/**
 * Service for displaying temperature on ZALMAN ALPHA 2 A36 liquid cooler.
 *
 * © 2025 Evgeniy Grigorev (evg.v.grigorev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * ZALMAN and product names are registered trademarks and are property of ZALMAN
 * TECH CO., Ltd and are used solely for reference and compatibility purposes.
 * Use of such trademarks does not imply any affiliation with or endorsement by
 * the trademark owners.
 */

#include "a36-display.h"
#include <getopt.h>
#include <libusb-1.0/libusb.h>
#include <sensors/sensors.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Supported devices
// Only first (ALPHA 2) is being checked by default without -n flag
static int DEVICES[3] = {0x3ed1, 0x4b8f, 0x3a7f};

static volatile sig_atomic_t keep_running = 1;

static void info(char *text) {
  fprintf(stdout, "a36-display: [info]  %s\n", text);
}

static void err(char *text) {
  fprintf(stderr, "a36-display: [error] %s\n", text);
}

static int init_usb_ctx(libusb_context **ctx) {
  int rc = libusb_init(ctx);

  if (rc != 0)
    return ERR_LUSB_INIT;

  libusb_set_option(*ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

  return 0;
}

static int usb_device_open(libusb_context *ctx, libusb_device_handle **handle,
                           int should_test_additional_hw) {
  // check only first device if no -n flag specified
  int length =
      should_test_additional_hw ? sizeof(DEVICES) / sizeof(DEVICES[0]) : 1;

  for (int i = 0; i < length; i++) {
    *handle = libusb_open_device_with_vid_pid(ctx, I_VENDOR_ID, DEVICES[i]);
    if (*handle) {
      fprintf(stdout, "a36-display: [info]  Working with %x\n", DEVICES[i]);
      return 0;
    }
  }

  return ERR_LUSB_HANDLE;
}

static int init_usb_interface(libusb_context *ctx, libusb_device_handle *handle,
                              int ifnum) {
  if (libusb_kernel_driver_active(handle, ifnum) == 1) {
    if (libusb_detach_kernel_driver(handle, ifnum) != 0) {
      libusb_close(handle);
      libusb_exit(ctx);

      return ERR_LUSB_DEV_DETACH;
    }
  }

  if (libusb_claim_interface(handle, ifnum) != 0) {
    return ERR_LUSB_CLAIM;
  }

  return 0;
}

static void release_usb_interface(libusb_device_handle *handle, int ifnum) {
  libusb_release_interface(handle, ifnum);
  libusb_attach_kernel_driver(handle, ifnum);
}

static int find_temp_sensor(sensors_chip_name **found_chip, int *num,
                            char *sensor_label) {
  int rc = sensors_init(NULL);

  if (rc != 0)
    return ERR_SENS_INIT;

  int chip_nr = 0;
  sensors_chip_name *chip;

  while ((chip = (sensors_chip_name *)sensors_get_detected_chips(
              NULL, &chip_nr)) != NULL) {
    const sensors_feature *feature;
    int feat_nr = 0;

    while ((feature = sensors_get_features(chip, &feat_nr)) != NULL) {
      if (feature->type != SENSORS_FEATURE_TEMP)
        continue;

      const char *label = sensors_get_label(chip, feature);
      if (strcmp(label, sensor_label) != 0)
        continue;

      int subfeat_nr = 0;
      const sensors_subfeature *sub;
      while ((sub = sensors_get_all_subfeatures(chip, feature, &subfeat_nr)) !=
             NULL) {
        if (sub->type == SENSORS_SUBFEATURE_TEMP_INPUT) {
          double val;
          if (sensors_get_value(chip, sub->number, &val) == 0) {
            *found_chip = chip;
            *num = sub->number;

            return rc;
          }
        }
      }
    }
  }

  return ERR_SENS_NOT_FOUND;
}

static int send_temp_over_usb(libusb_device_handle *handle,
                              sensors_chip_name *chip, int subfeat_num,
                              int *transferred) {
  unsigned char payload[I_PAYLOAD_SIZE];
  double temp;
  int rc = sensors_get_value(chip, subfeat_num, &temp);

  if (!rc) {
    memset(payload, 0, sizeof payload);

    payload[0] = I_CMD_BYTE;
    payload[1] = (int)temp;

    return libusb_bulk_transfer(handle, I_OUT_ENDPOINT,
                                (unsigned char *)payload, sizeof(payload),
                                transferred, I_USB_TIMEOUT);
  }

  return rc;
}

static void handle_sigint(int _) {
  (void)_;
  keep_running = 0;
}

int main(int argc, char *argv[]) {
  extern char *optarg;
  char *sensor_label = "CPU";

  int iterations = 0;
  int opt;
  int rc = 0;
  int subfeat_num = 0;
  int transferred = 0;
  int untested_hw = 0;

  libusb_context *ctx = NULL;
  libusb_device_handle *handle = NULL;

  sensors_chip_name *chip;

  signal(SIGINT, handle_sigint);

  while ((opt = getopt(argc, argv, "t:l:nh")) != -1) {
    switch (opt) {
    // `t` arg allows to run service for a specified amount of usb send
    // ticks, istead of running indefinitely
    // basically is a run duration in seconds
    case 't': {
      iterations = atoi(optarg);
    } break;
    // `l` arg is a label of lm_sensor to use
    case 'l': {
      sensor_label = optarg;
    } break;
    // `n` arg is the untested_hw flag, which allows this service to
    // run on hardware it should be compatible with but has not yet
    // been tested
    case 'n': {
      info("Scan for additional devices");
      untested_hw = 1;
    } break;
    case 'h': {
      puts("Usage: a36-display [OPTION]");
      puts("  -t  amount of packages to send via USB (1 pkg per sec)");
      puts("  -l  label of sensor to use as a data source");
      puts("  -n  try to work with presumably supported devices");

      return 0;
    } break;
    }
  }

  rc = init_usb_ctx(&ctx);

  if (!rc) {
    rc = usb_device_open(ctx, &handle, untested_hw);

    if (!rc) {
      rc = init_usb_interface(ctx, handle, I_USB_INTERFACE);

      if (!rc) {
        // Set-Idle request must be sent first, no data is required
        libusb_control_transfer(handle, I_IDLE_BM_REQUEST_TYPE, I_IDLE_REQUEST,
                                0, 0x00, nullptr, 0, I_USB_TIMEOUT);

        rc = find_temp_sensor(&chip, &subfeat_num, sensor_label);

        if (!rc) {
          rc = send_temp_over_usb(handle, chip, subfeat_num, &transferred);

          if (!rc && transferred > 0) {
            int i = 0;

            while (keep_running && (i < iterations || !iterations)) {
              rc = send_temp_over_usb(handle, chip, subfeat_num, &transferred);

              if (rc)
                break;

              usleep(I_SLEEP_INTERVAL);

              if (iterations)
                i++;
            }
          }
        }

        sensors_cleanup();
        release_usb_interface(handle, I_USB_INTERFACE);
      }

      libusb_close(handle);
    }

    libusb_exit(ctx);
  }

  switch (rc) {
  case ERR_SENS_NOT_FOUND:
    err("No appropriate sensor found");
    break;
  case ERR_SENS_INIT:
    err("Can not initialize sensors");
    break;
  case ERR_LUSB_CLAIM:
    err("Can not claim USB device interface");
    break;
  case ERR_LUSB_DEV_DETACH:
    err("Can not detach USB device from kernel");
    break;
  case ERR_LUSB_HANDLE:
    err("No supported devices found or no access to USB devices, try using "
        "sudo or -n flag");
    break;
  case ERR_LUSB_INIT:
    err("Can not initialize libusb");
    break;
  default:
    info("Exiting");
  }

  return rc;
}
