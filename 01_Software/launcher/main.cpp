// Out Of Fun Dashboard kiosk: one full-screen WPE WebKit view using the WPEPlatform API.
//
// Unlike Cog, this does not go through libwpe and wpebackend-fdo: WebKit connects to the
// Wayland compositor itself and hands it dmabuf buffers directly.
//
// Usage: dashboard-launcher URL   (or set DASHBOARD_URL)

#include <wpe/webkit.h>

#include <cstdlib>

namespace {

GMainLoop* mainLoop = nullptr;

void onWebProcessTerminated(WebKitWebView* view, WebKitWebProcessTerminationReason reason, gpointer)
{
    g_warning("Web process terminated (reason %d), reloading the page", reason);
    webkit_web_view_reload(view);
}

gboolean onLoadFailed(WebKitWebView*, WebKitLoadEvent, gchar* uri, GError* error, gpointer)
{
    g_warning("Failed to load %s: %s", uri, error->message);
    return FALSE;
}

void onDisplayDisconnected(WPEDisplay*, GError* error, gpointer)
{
    // Weston went away; exit so systemd can start us again together with it.
    g_warning("Display disconnected: %s", error ? error->message : "unknown reason");
    g_main_loop_quit(mainLoop);
}

} // namespace

int main(int argc, char** argv)
{
    const char* uri = argc > 1 ? argv[1] : g_getenv("DASHBOARD_URL");
    if (!uri || !*uri) {
        g_printerr("Usage: %s URL (or set DASHBOARD_URL)\n", argv[0]);
        return EXIT_FAILURE;
    }

    mainLoop = g_main_loop_new(nullptr, FALSE);

    WebKitSettings* settings = webkit_settings_new_with_settings(
        "enable-write-console-messages-to-stdout", TRUE,
        "enable-webgl", TRUE,
        nullptr);

    // Without a "backend" property WebKit uses the WPEPlatform API with the default
    // display, which is Wayland when WAYLAND_DISPLAY is set.
    auto* view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, "settings", settings, nullptr));
    g_object_unref(settings);

    WPEView* wpeView = webkit_web_view_get_wpe_view(view);
    if (!wpeView) {
        g_printerr("WPEPlatform view is not available\n");
        return EXIT_FAILURE;
    }
    g_signal_connect(wpe_view_get_display(wpeView), "disconnected", G_CALLBACK(onDisplayDisconnected), nullptr);
    if (WPEToplevel* toplevel = wpe_view_get_toplevel(wpeView))
        wpe_toplevel_fullscreen(toplevel);

    g_signal_connect(view, "web-process-terminated", G_CALLBACK(onWebProcessTerminated), nullptr);
    g_signal_connect(view, "load-failed", G_CALLBACK(onLoadFailed), nullptr);

    webkit_web_view_load_uri(view, uri);
    g_main_loop_run(mainLoop);

    g_object_unref(view);
    g_main_loop_unref(mainLoop);
    return EXIT_SUCCESS;
}
