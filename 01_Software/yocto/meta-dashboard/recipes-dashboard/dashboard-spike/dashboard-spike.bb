SUMMARY = "Rendering benchmark page for the dashboard"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# The page lives in the main repository, outside the layer: 01_Software/spike.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../spike:"
SRC_URI = "file://index.html"
S = "${UNPACKDIR}"

inherit allarch

do_install() {
    install -Dm 0644 ${S}/index.html ${D}${datadir}/dashboard-spike/index.html
}

FILES:${PN} = "${datadir}/dashboard-spike"
