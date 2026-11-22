SUMMARY = "Persistent /data partition and RAUC integration for the dashboard"
DESCRIPTION = "Grows /data to the end of the SD card, keeps the SSH host key on /data \
and makes RAUC wait for /data (docs/ota-rauc.md)."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://dashboard-grow-data \
    file://dashboard-grow-data.service \
    file://dropbearkey.conf \
    file://dropbear.conf \
    file://rauc.conf \
"
S = "${UNPACKDIR}"

inherit systemd allarch

do_install() {
    install -Dm 0755 ${S}/dashboard-grow-data ${D}${libexecdir}/dashboard-grow-data
    install -Dm 0644 ${S}/dashboard-grow-data.service ${D}${systemd_system_unitdir}/dashboard-grow-data.service
    install -Dm 0644 ${S}/dropbearkey.conf ${D}${systemd_system_unitdir}/dropbearkey.service.d/data.conf
    install -Dm 0644 ${S}/dropbear.conf ${D}${systemd_system_unitdir}/dropbear@.service.d/data.conf
    install -Dm 0644 ${S}/rauc.conf ${D}${systemd_system_unitdir}/rauc.service.d/data.conf
}

SYSTEMD_SERVICE:${PN} = "dashboard-grow-data.service"
FILES:${PN} += "${systemd_system_unitdir}"
RDEPENDS:${PN} = "parted"
