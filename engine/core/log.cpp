#include "log.h"

Logger g_logger;

void log_init(const char *filename) { g_logger.init(filename); }
void log_release() { g_logger.release(); }
