/*
 * gang_spdlog_console.h
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#ifndef GANG_SPDLOG_CONSOLE_H_
#define GANG_SPDLOG_CONSOLE_H_

#include "spdlog/spdlog.h"

namespace gang {

extern std::shared_ptr<spdlog::logger> console;

void InitGangSpdlogConsole();

void CleanupGangSpdlog();

} // namespace gang

#endif /* GANG_SPDLOG_CONSOLE_H_ */
