// The HTTP side: the frontend's files and the /api of docs/plan.md. The only file that
// includes cpp-httplib.
#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "display.hpp"
#include "hub.hpp"
#include "lights.hpp"

namespace dash {

class HttpServer {
public:
  HttpServer(Hub &hub, Lights &lights, Display &display, const std::filesystem::path &web_dir);
  ~HttpServer();

  /** Binds the port; connections queue up from here on. False when the port is taken. */
  bool bind(const std::string &host, int port);

  /** Serves until stop(). */
  void run();

  /** Ends run(); safe from another thread. */
  void stop();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace dash
