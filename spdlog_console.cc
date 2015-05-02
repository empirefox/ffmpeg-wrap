/*
 * spdlog_console.cc
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#include "spdlog_console.h"

namespace gang {

void InitSpdlogConsole() {
	console = spdlog::stdout_logger_mt("console");
}

void CleanupSpdlog() {
	console = NULL;
}

} // namespace gang
