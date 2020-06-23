#ifndef UDPTEST_COMMON_H_
#define UDPTEST_COMMON_H_

#ifdef UDPTEST_DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

// spdlog includes
#include <spdlog/spdlog.h>

#endif