# a36-display

A simple utility to substitute ZALMAN-CTM app on Linux. Allows to display temperature value on screen of ALPHA 2 A36 liquid cooler. Should also be compatible with several other ZALMAN devices, but this requires testing yet.

**This software is not an official port of ZALMAN-CTM app and is unrelated to ZALMAN TECH CO., Ltd.** For details, see the Legal disclaimer section of this document.

**Special thanks** to ZALMAN for permission to publish this program ❤️

## Compatible devices

### Definitely compatible
* ZALMAN ALPHA 2 A36

### Should be compatible, requires testing
* ZALMAN ALPHA 2 A24
* ZALMAN CNPS13X DS

### Should be partially compatible, requires testing
* ZALMAN CNPS9X ECO DS (will show only temperature)

## Requirements
* libusb
* lm_sensors

## Installation guide

1. Install dependencies listed above with your package manager
1. Generate systemd service with `make systemd-service`
1. Edit a36-display.service according to your preferences (see below)
1. Build, isntall and run service with `make install`

Use `make remove` to remove service and its symlinks from your system.

## Data source

This program uses `lm_sensors` to detect the tempreature (meaning CPU temperature by default). Since there's no absolutely reliable way to always detect the correct sensor, I've decided to leave this up to user.

To specify which sensor should be used to display its value, you can pass its label with `-l` flag to binary in `ExecStart` field of `a36-display.service` file you've generated with `make systemd-service`:

*Example 1:*
```
ExecStart=/usr/bin/a36-display -l CPU
```

*Example 2:*
```
ExecStart=/usr/bin/a36-display -l "Core 1"
```

By default program will try to use sensor labeled as CPU like in *Example 1*.

## Enabling with presumably compatible devices

By default, the program only detects the A36 liquid cooling system, as it was developed specifically for it, but it should presumably work with the devices mentioned above as well.

Basically, if your device is recommended to work with the ZALMAN-CTM application and it has a two-digit LED display, then this program should work with it.

To test your device with program without installing it into your system you can do the following:

```
make && ./dist/a36-display -n -t 5 -l %your_device_temp_sensor_label%
```

Where the `-t` flag is the number of times the program will try to send data to the screen (in other words, how many seconds the display should stay active, since program updates it once per second).

If there are no errors in CLI and you see it working, go through the instruction in an Installation guide section and add `-n` flag to your service file on step 2:

*Example:*
```
ExecStart=/usr/bin/a36-display -n
```

## Legal disclaimer

This software is an independently developed third‑party driver for proprietary hardware manufactured by the ZALMAN TECH CO., Ltd. It is provided and maintained by its author and is not produced, endorsed, sponsored, or supported by the ZALMAN TECH CO., Ltd.

ZALMAN TECH CO., Ltd. has given permission for this software to be published, but is not affiliated with the author and bears no responsibility for this software’s functionality, quality, or suitability for any purpose.

All trademarks, trade names, logos and other proprietary designations mentioned in this software, its documentation, or related materials are the property of their respective owners and are used solely for reference and compatibility purposes. Use of such trademarks does not imply any affiliation with or endorsement by the trademark owners.

By using this software, you acknowledge and agree that:
* the author and any contributors are solely responsible for this software; and
* the ZALMAN TECH CO., Ltd. has no obligation to provide support, updates, warranties, or indemnities related to this software.

If you require official support, warranty service, or other assistance for the hardware, contact the ZALMAN TECH CO., Ltd. directly.
