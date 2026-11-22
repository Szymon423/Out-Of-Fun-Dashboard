SUMMARY = "Persistent /data, RAUC integration and boot health check for the dashboard"
DESCRIPTION = "Grows /data to the end of the SD card, keeps the SSH host key on /data, makes RAUC \
wait for /data, enables the hardware watchdog and checks every boot before RAUC marks the slot \
as good (docs/ota-rauc.md)."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://dashboard-grow-data \
    file://dashboard-grow-data.service \
    file://dashboard-health \
    file://dashboard-health.service \
    file://dropbearkey.conf \
    file://dropbear.conf \
    file://rauc.conf \
    file://watchdog.conf \
"
S = "${UNPACKDIR}"

inherit systemd allarch

do_install() {
    install -Dm 0755 ${S}/dashboard-grow-data ${D}${libexecdir}/dashboard-grow-data
    install -Dm 0644 ${S}/dashboard-grow-data.service ${D}${systemd_system_unitdir}/dashboard-grow-data.service
    install -Dm 0755 ${S}/dashboard-health ${D}${libexecdir}/dashboard-health
    install -Dm 0644 ${S}/dashboard-health.service ${D}${systemd_system_unitdir}/dashboard-health.service
    install -Dm 0644 ${S}/dropbearkey.conf ${D}${systemd_system_unitdir}/dropbearkey.service.d/data.conf
    install -Dm 0644 ${S}/dropbear.conf ${D}${systemd_system_unitdir}/dropbear@.service.d/data.conf
    install -Dm 0644 ${S}/rauc.conf ${D}${systemd_system_unitdir}/rauc.service.d/data.conf
    install -Dm 0644 ${S}/watchdog.conf ${D}${systemd_unitdir}/system.conf.d/dashboard-watchdog.conf
}

SYSTEMD_SERVICE:${PN} = "dashboard-grow-data.service dashboard-health.service"
# Units, drop-ins and the system.conf.d snippet all live under /usr/lib/systemd.
FILES:${PN} += "${systemd_unitdir}"
RDEPENDS:${PN} = "parted"
