SUMMARY = "Out Of Fun Dashboard image"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear allow-root-login"

IMAGE_INSTALL = " \
        packagegroup-core-boot \
        packagegroup-base-extended \
        ${CORE_IMAGE_EXTRA_INSTALL} \
        dashboard-ssh-key \
        iproute2 \
        procps \
        tzdata \
"

# Weston kiosk with the WPE launcher showing the dashboard from dashboard-server; the
# spike pages stay for spike/diag.sh.
IMAGE_FEATURES += "weston"
IMAGE_INSTALL += "dashboard-kiosk dashboard-server dashboard-web dashboard-spike ttf-dejavu-sans ttf-dejavu-sans-mono bash"

# fw_printenv and fw_setenv for the U-Boot environment, used by RAUC.
IMAGE_INSTALL += "libubootenv-bin u-boot-env"

# A/B layout for RAUC (docs/ota-rauc.md). The kernel lives in /boot of each rootfs slot,
# so it is updated and rolled back together with its modules; U-Boot loads it from there.
WKS_FILE = "dashboard-ab.wks"
IMAGE_INSTALL += "kernel-image rauc dashboard-ota"
RPI_EXTRA_IMAGE_BOOT_FILES:remove = "${KERNEL_IMAGETYPE}"
# ext4 is the rootfs image for update bundles, wic is for flashing the card.
IMAGE_FSTYPES = "ext4 wic.bz2 wic.bmap"
