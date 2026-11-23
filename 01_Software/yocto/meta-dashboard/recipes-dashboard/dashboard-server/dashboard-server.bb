SUMMARY = "Dashboard server: the frontend and its /api on 127.0.0.1:8080"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# The source lives in the main repository, outside the layer: 01_Software/dashboard/server.
# cpp-httplib (MIT) is part of it, in third_party/.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../dashboard:"
SRC_URI = " \
    file://server \
    file://dashboard-server.service \
    file://dashboard-server.conf \
    file://90-dashboard-backlight.rules \
"
S = "${UNPACKDIR}/server"

DEPENDS = "curl nlohmann-json paho-mqtt-c libical"

inherit meson pkgconfig systemd useradd

do_install:append() {
    install -Dm 0644 ${UNPACKDIR}/dashboard-server.service \
        ${D}${systemd_system_unitdir}/dashboard-server.service
    install -Dm 0644 ${UNPACKDIR}/dashboard-server.conf \
        ${D}${nonarch_libdir}/tmpfiles.d/dashboard-server.conf
    install -Dm 0644 ${UNPACKDIR}/90-dashboard-backlight.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/90-dashboard-backlight.rules
}

# The server runs as its own user; its data is /data/dashboard.
USERADD_PACKAGES = "${PN}"
USERADD_PARAM:${PN} = "--system --no-create-home --home-dir /data/dashboard \
    --shell /sbin/nologin --user-group dashboard"

SYSTEMD_SERVICE:${PN} = "dashboard-server.service"
FILES:${PN} += "${nonarch_libdir}/tmpfiles.d ${nonarch_base_libdir}/udev/rules.d"
# ca-certificates: HTTPS to Open-Meteo, later also Immich, Spotify and Microsoft.
RDEPENDS:${PN} = "ca-certificates dashboard-web"
