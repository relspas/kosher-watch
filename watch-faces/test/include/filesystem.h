#pragma once

#include <stdbool.h>
#include <stdint.h>

bool filesystem_read_file(char *filename, char *buf, int32_t length);
bool filesystem_write_file(char *filename, char *buf, int32_t length);
