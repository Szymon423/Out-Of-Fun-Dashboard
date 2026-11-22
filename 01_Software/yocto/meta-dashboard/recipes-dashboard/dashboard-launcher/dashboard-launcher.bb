SUMMARY = "Full-screen WPE WebKit kiosk launcher using the WPEPlatform API"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# The source lives in the main repository, outside the layer: 01_Software/launcher.
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../..:"
SRC_URI = "file://launcher"
S = "${UNPACKDIR}/launcher"

DEPENDS = "wpewebkit"

inherit meson pkgconfig
