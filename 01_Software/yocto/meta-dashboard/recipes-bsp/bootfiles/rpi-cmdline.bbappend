# Kernel console on the screen. U-Boot needs enable_uart=1, and without console= the
# kernel follows stdout-path in the device tree to the UART: 115200 baud slowed the
# boot by about 4 s and systemd started a login getty on ttyS0.
CMDLINE:append = " console=tty1"
