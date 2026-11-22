SUMMARY = "Out Of Fun Dashboard image"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear allow-root-login"

IMAGE_INSTALL = " \
	packagegroup-core-boot \
	packagegroup-base-extended \
	${CORE_IMAGE_EXTRA_INSTALL} \
	dashboard-ssh-key \
	iproute2 \
	procps \
	tzdata \
"

# Weston kiosk with the WPE launcher showing the benchmark page.
IMAGE_FEATURES += "weston"
IMAGE_INSTALL += "dashboard-kiosk dashboard-spike ttf-dejavu-sans ttf-dejavu-sans-mono bash"
