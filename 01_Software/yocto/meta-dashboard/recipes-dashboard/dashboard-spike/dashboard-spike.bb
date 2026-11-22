SUMMARY = "Benchmark and touch test pages for the dashboard"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# The pages live in the main repository, outside the layer: 01_Software/spike.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../spike:"
SRC_URI = "file://index.html file://stress.html"
S = "${UNPACKDIR}"

inherit allarch

do_install() {
    install -Dm 0644 ${S}/index.html ${D}${datadir}/dashboard-spike/index.html
    install -Dm 0644 ${S}/stress.html ${D}${datadir}/dashboard-spike/stress.html
}

FILES:${PN} = "${datadir}/dashboard-spike"
