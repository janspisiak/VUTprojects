#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdexcept>
#include <string>

#include "Slog.h"

// Shortcut for declaring str in arguments
#define CSTR_ARG(str) str, sizeof(str) - 1

std::string substrr(std::string const& str, size_t beg, size_t end);

std::string findAndReplace(const std::string& source, const std::string& find, const std::string& replace);

#endif