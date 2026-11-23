# dashboard-server: the dashboard's local HTTP server (docs/plan.md). On the Pi it listens
# on 127.0.0.1:8080, serves the frontend and the /api the frontend reads.
project(
  'dashboard-server',
  'cpp',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>= 1.3',
  default_options: ['cpp_std=c++20', 'warning_level=3', 'buildtype=release', 'b_ndebug=if-release'],
)

curl = dependency('libcurl')
json = dependency('nlohmann_json')
ical = dependency('libical') # calendars: ICS parsing and recurring events
threads = dependency('threads')
# Eclipse Paho MQTT C, the synchronous client with TLS: the Bambu printer (printer.hpp).
# Neither Fedora's nor Yocto's build ships a pkg-config file for it.
cpp = meson.get_compiler('cpp')
paho = declare_dependency(dependencies: cpp.find_library('paho-mqtt3cs'))
if not cpp.has_header('MQTTClient.h')
  error('MQTTClient.h not found: install paho-c-devel (Fedora) or add paho-mqtt-c to DEPENDS (Yocto)')
endif

# One header, kept in the tree (third_party/cpp-httplib/README.md). Included only by
# http_api.cpp, because it takes a while to compile.
httplib = declare_dependency(
  include_directories: include_directories('third_party/cpp-httplib', is_system: true),
)

core = static_library(
  'dashboard-core',
  'src/calendar.cpp',
  'src/config.cpp',
  'src/display.cpp',
  'src/fetch.cpp',
  'src/hub.cpp',
  'src/immich.cpp',
  'src/lights.cpp',
  'src/log.cpp',
  'src/printer.cpp',
  'src/spotify.cpp',
  'src/weather.cpp',
  dependencies: [curl, json, threads, paho, ical],
)
core_dep = declare_dependency(
  link_with: core,
  dependencies: [curl, json, threads, paho, ical],
  include_directories: include_directories('src'),
)

executable(
  'dashboard-server',
  'src/http_api.cpp',
  'src/main.cpp',
  'src/systemd.cpp',
  dependencies: [core_dep, httplib],
  install: true,
)

# Built only by `meson test`, not by the image build.
test(
  'weather',
  executable('weather-test', 'tests/weather_test.cpp', dependencies: core_dep, build_by_default: false),
  args: [files('tests/open-meteo.json')],
)
test(
  'immich',
  executable('immich-test', 'tests/immich_test.cpp', dependencies: core_dep, build_by_default: false),
)
test(
  'spotify',
  executable('spotify-test', 'tests/spotify_test.cpp', dependencies: core_dep, build_by_default: false),
)
test(
  'printer',
  executable('printer-test', 'tests/printer_test.cpp', dependencies: core_dep, build_by_default: false),
)
test(
  'calendar',
  executable('calendar-test', 'tests/calendar_test.cpp', dependencies: core_dep, build_by_default: false),
)
test(
  'hub',
  executable('hub-test', 'tests/hub_test.cpp', dependencies: core_dep, build_by_default: false),
)
