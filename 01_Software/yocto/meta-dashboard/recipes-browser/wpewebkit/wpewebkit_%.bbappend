# The kiosk uses the WPEPlatform API (dashboard-launcher), so the old libwpe API
# and its wpebackend-fdo backend are not needed.
PACKAGECONFIG:remove = "wpe-legacy-api"
