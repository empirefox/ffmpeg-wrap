/*
 * gang_spdlog_console.cc
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#include "gang_spdlog_console.h"

namespace gang {

std::shared_ptr<spdlog::logger> console = NULL;

void InitGangSpdlogConsole() {
	console = spdlog::stdout_logger_mt("console:gang");
}

void CleanupGangSpdlog() {
	console = NULL;
}

} // namespace gang
