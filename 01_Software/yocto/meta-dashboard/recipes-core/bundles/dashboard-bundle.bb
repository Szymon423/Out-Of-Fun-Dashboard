SUMMARY = "Out Of Fun Dashboard update bundle"
DESCRIPTION = "Signed RAUC bundle with the dashboard-image rootfs for slot A or B (docs/ota-rauc.md). \
Install it with tools/ota.sh."
LICENSE = "MIT"

inherit bundle

# Must match compatible= in recipes-core/rauc/files/system.conf, or the Pi refuses the bundle.
RAUC_BUNDLE_COMPATIBLE = "out-of-fun-dashboard-rpi4"
RAUC_BUNDLE_VERSION = "${DISTRO_VERSION}"
RAUC_BUNDLE_FORMAT = "verity"

RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs = "dashboard-image"
# The bundle recipe sees the machine's IMAGE_FSTYPES (tar.bz2 first), not the image's.
RAUC_SLOT_rootfs[fstype] = "ext4"

# zstd: smaller than the default gzip and faster to unpack on the Pi (kernel has SQUASHFS_ZSTD).
BUNDLE_ARGS += '--mksquashfs-args="-comp zstd -Xcompression-level 19"'

# Signing key and certificate from tools/rauc-keys.sh, kept outside git in 01_Software/yocto/keys/.
RAUC_KEY_FILE = "${TOPDIR}/../keys/openssl-ca/dev/private/development-1.key.pem"
RAUC_CERT_FILE = "${TOPDIR}/../keys/openssl-ca/dev/development-1.cert.pem"
