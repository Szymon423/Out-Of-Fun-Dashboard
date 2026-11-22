# Our system.conf and the public certificate of our CA (from tools/rauc-keys.sh), both
# installed to /usr/lib/rauc (docs/ota-rauc.md).
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# system.conf sets resize=true, so RAUC runs resize2fs after writing a slot; the image
# has no resize2fs otherwise and the first update would fail.
RDEPENDS:${PN} += "e2fsprogs-resize2fs"
