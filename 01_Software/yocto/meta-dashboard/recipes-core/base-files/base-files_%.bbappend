# fstab for the A/B layout and mount points for the boot and data partitions (docs/ota-rauc.md).
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
dirs755 += "/boot/firmware /data"
