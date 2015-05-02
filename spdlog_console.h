/*
 * spdlog_console.h
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#ifndef SPDLOG_CONSOLE_H_
#define SPDLOG_CONSOLE_H_

#include "spdlog/spdlog.h"

namespace gang {

std::shared_ptr<spdlog::logger> console;

void InitSpdlogConsole();

void CleanupSpdlog();

} // namespace gang

#endif /* SPDLOG_CONSOLE_H_ */
