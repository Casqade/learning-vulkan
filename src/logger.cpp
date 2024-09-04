#include "logger.hpp"

#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>


std::shared_ptr <spdlog::logger>
createLogger()
{
  using spdlog::sinks::basic_file_sink_mt;
  using spdlog::sinks::stdout_color_sink_mt;
  using log_level = spdlog::level::level_enum;

  const auto loggerName = "vulkan_sandbox";

  struct
    {
      struct
      {
        log_level cmd {log_level::trace};
        log_level file {log_level::trace};
        log_level flushOn {log_level::trace};

      } level {};

      std::string pattern {"[%T.%e] [%^%l%$] [%s:%#] [tr %t] %v"};

    } log {};


  auto logger = spdlog::get(loggerName);

  if ( logger != nullptr )
    return logger;


  const std::string executableName = "vulkan_sandbox";
  const auto logFilename = executableName + ".log";

  std::vector <std::shared_ptr <spdlog::sinks::sink>> sinks {};

  auto logLevel = log.level.cmd;

  if ( logLevel < log_level::off )
  {
    auto stdoutSink = std::make_shared <stdout_color_sink_mt> ();
    stdoutSink->set_level(logLevel);

    sinks.push_back(stdoutSink);
  }

  logLevel = log.level.file;

  if ( logLevel < log_level::off )
  {
    try
    {
      auto fileSink = std::make_shared <basic_file_sink_mt> (logFilename, true);
      fileSink->set_level(logLevel);
      sinks.push_back(fileSink);
    }
    catch ( const std::exception& e )
    {
      using fmt::format;
      std::cerr <<
        format(
          "Error: Can't create file sink for logger ({}). "
          "Log output to file will be turned off\n",
          e.what() );
    }
  }

  auto dist_sink = std::make_shared <spdlog::sinks::dist_sink_mt> (sinks);
  dist_sink->set_level(spdlog::level::trace);

  spdlog::init_thread_pool(spdlog::details::default_async_q_size, 1);
//  auto logger = std::make_shared <spdlog::async_logger> (
//    loggerName, dist_sink,
//    spdlog::thread_pool() );

  logger = std::make_shared <spdlog::logger> (loggerName, dist_sink);
  logger->set_level(spdlog::level::trace);
  logger->set_pattern(log.pattern);
  logger->flush_on(log.level.flushOn);

  spdlog::register_logger(logger);
  spdlog::set_default_logger(logger);

  LOG_INFO("logger initialized");

  return logger;
}

