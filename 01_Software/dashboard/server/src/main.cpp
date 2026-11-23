// dashboard-server: serves the dashboard frontend and its /api on the Pi (docs/plan.md).
#include <curl/curl.h>
#include <pthread.h>
#include <signal.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>

#include "config.hpp"
#include "display.hpp"
#include "http_api.hpp"
#include "hub.hpp"
#include "lights.hpp"
#include "log.hpp"
#include "systemd.hpp"
#include "weather.hpp"

namespace fs = std::filesystem;
using namespace dash;

namespace {

constexpr const char *kUsage = R"(Usage: dashboard-server [options]
  --listen HOST:PORT   address to serve on (default 127.0.0.1:8080)
  --web DIR            the built frontend (default /usr/share/dashboard/web)
  --data DIR           config.json and saved state, kept across updates (default /data/dashboard)
  --backlight WHAT     auto: the first /sys/class/backlight device; none: keep the brightness
                       only in memory (default); or the sysfs directory of the device
)";

struct Options {
  std::string host = "127.0.0.1";
  int port = 8080;
  fs::path web = "/usr/share/dashboard/web";
  fs::path data = "/data/dashboard";
  std::string backlight = "none";
};

[[noreturn]] void usage(int code) {
  std::fputs(kUsage, code ? stderr : stdout);
  std::exit(code);
}

Options parse(int argc, char **argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") usage(0);
    if (i + 1 >= argc) usage(2);
    const std::string value = argv[++i];
    if (arg == "--listen") {
      const auto colon = value.rfind(':');
      if (colon == std::string::npos) usage(2);
      o.host = value.substr(0, colon);
      o.port = std::stoi(value.substr(colon + 1));
    } else if (arg == "--web") {
      o.web = value;
    } else if (arg == "--data") {
      o.data = value;
    } else if (arg == "--backlight") {
      o.backlight = value;
    } else {
      usage(2);
    }
  }
  return o;
}

std::optional<fs::path> backlight_dir(const std::string &what) {
  if (what == "none") return std::nullopt;
  if (what == "auto") {
    auto found = Display::find_backlight();
    if (!found) log_warning("display: no device in /sys/class/backlight");
    return found;
  }
  return fs::path(what);
}

} // namespace

int main(int argc, char **argv) {
  const Options options = parse(argc, argv);

  // SIGTERM and SIGINT go to one thread that stops the server; block them everywhere else.
  sigset_t stop_signals;
  sigemptyset(&stop_signals);
  sigaddset(&stop_signals, SIGTERM);
  sigaddset(&stop_signals, SIGINT);
  pthread_sigmask(SIG_BLOCK, &stop_signals, nullptr);

  curl_global_init(CURL_GLOBAL_DEFAULT);

  std::error_code ec;
  fs::create_directories(options.data, ec);
  if (ec) log_warning("cannot create {}: {}", options.data.string(), ec.message());

  // A broken config must not stop the server: the health check would then reject this
  // update, and the previous slot reads the same /data. Run without the broken parts.
  Config config;
  try {
    config = load_config(options.data);
  } catch (const std::exception &e) {
    log_error("config: {}; running without it", e.what());
  }

  Hub hub;
  Lights lights(hub, options.data / "lights.json");
  Display display(hub, backlight_dir(options.backlight));
  std::optional<WeatherProvider> weather;
  if (config.location.empty()) {
    log_warning("weather: no location in {}/config.json, no forecast", options.data.string());
  } else {
    weather.emplace(hub, config.location);
  }

  HttpServer http(hub, lights, display, options.web);
  if (!http.bind(options.host, options.port)) {
    log_error("cannot listen on {}:{}: {}", options.host, options.port, std::strerror(errno));
    return 1;
  }
  log_info("serving http://{}:{}/ (frontend {}, data {})", options.host, options.port, options.web.string(),
           options.data.string());
  sd_notify("READY=1");

  std::atomic<bool> stopping = false;
  std::thread signals([&] {
    int signal = 0;
    sigwait(&stop_signals, &signal);
    if (!stopping) log_info("{}, stopping", signal == SIGTERM ? "SIGTERM" : "SIGINT");
    stopping = true;
    sd_notify("STOPPING=1");
    hub.close(); // ends the open event streams
    http.stop();
  });

  http.run();
  if (!stopping.exchange(true)) pthread_kill(signals.native_handle(), SIGTERM); // wake it to end
  signals.join();

  weather.reset();
  curl_global_cleanup();
  return 0;
}
