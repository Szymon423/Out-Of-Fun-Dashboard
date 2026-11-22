# U-Boot environment in raw SD card sectors, fast autoboot and boot timing (docs/ota-rauc.md, OTA-1).
# files/fw_env.config replaces meta-raspberrypi's, which points to /boot/uboot.env.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://env-in-mmc.cfg file://autoboot.cfg file://bootstage.cfg"
