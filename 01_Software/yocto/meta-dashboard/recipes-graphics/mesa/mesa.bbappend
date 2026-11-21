# Cog draws through wpebackend-fdo, which needs EGL_WL_bind_wayland_display.
# Mesa 26 no longer builds it by default; without it WebKit falls back to the CPU.
EXTRA_OEMESON:append = " -Dlegacy-wayland=bind-wayland-display"
