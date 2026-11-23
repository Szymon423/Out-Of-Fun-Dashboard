SUMMARY = "Dashboard frontend, served by dashboard-server"
DESCRIPTION = "The Svelte frontend, built on the PC by build.sh (npm run build) and packaged as it is."
# Our code and Svelte: MIT. Lucide icons: ISC. Inter and the clock fonts: OFL-1.1.
LICENSE = "MIT & ISC & OFL-1.1"
LIC_FILES_CHKSUM = " \
    file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302 \
    file://${COMMON_LICENSE_DIR}/ISC;md5=f3b90e78ea0cffb20bf5cca7947a896d \
    file://${COMMON_LICENSE_DIR}/OFL-1.1;md5=fac3a519e5e9eb96316656e0ca4f2b90 \
"

# web/dist from the main repository, outside the layer: 01_Software/dashboard/web/dist.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../dashboard:"
SRC_URI = "file://web/dist"
S = "${UNPACKDIR}/web/dist"

inherit allarch

do_install() {
    install -d ${D}${datadir}/dashboard/web
    cp -R --no-preserve=ownership ${S}/. ${D}${datadir}/dashboard/web/
}

FILES:${PN} = "${datadir}/dashboard/web"
