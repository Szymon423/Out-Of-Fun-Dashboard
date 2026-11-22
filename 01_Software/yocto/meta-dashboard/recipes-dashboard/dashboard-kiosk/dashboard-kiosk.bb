SUMMARY = "Starts the dashboard launcher full screen in Weston on boot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dashboard-kiosk.service \
           file://dashboard-kiosk.env"
S = "${UNPACKDIR}"

inherit systemd allarch

do_install() {
    install -Dm 0644 ${S}/dashboard-kiosk.service ${D}${systemd_system_unitdir}/dashboard-kiosk.service
    install -Dm 0644 ${S}/dashboard-kiosk.env ${D}${sysconfdir}/default/dashboard-kiosk
}

SYSTEMD_SERVICE:${PN} = "dashboard-kiosk.service"
CONFFILES:${PN} = "${sysconfdir}/default/dashboard-kiosk"
RDEPENDS:${PN} = "dashboard-launcher weston-init"
