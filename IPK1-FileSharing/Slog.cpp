#include "Slog.h"

const char* Logger::SeverityLetter[] = {
	"E",
	"W",
	"N",
	"D"
};

Logger::Severity Logger::verbosity = Logger::Severity::Warning;

std::ostringstream & Logger::log(Severity level)
{
	stream << SeverityLetter[level] << ": ";
	return stream;
}

Logger::~Logger()
{
	std::string str = stream.str();
	while (std::isspace(str.back()))
		str.pop_back();
	str.push_back('\n');
	fprintf(stderr, "%s", str.c_str());
	fflush(stderr);
}