#ifndef SLOG_H
#define SLOG_H
#endif

#include <sstream>
#include <cstdio>

#ifndef SLOG_MAX_LEVEL
#define SLOG_MAX_LEVEL Logger::Severity::Debug
#endif

#define Slog(LEVEL) \
	if (Logger::Severity::LEVEL <= SLOG_MAX_LEVEL) \
		if (Logger::Severity::LEVEL <= Logger::verbosity) \
			Logger().log(Logger::Severity::LEVEL)

class Logger {
		std::ostringstream stream;
	public:
		enum Severity {
			Error,
			Warning,
			Notice,
			Debug
		};

		static const char* SeverityLetter[];
		
		static Severity verbosity;

		std::ostringstream & log(Severity level);
		~Logger();
};