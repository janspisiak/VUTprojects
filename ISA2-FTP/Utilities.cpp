#include "Utilities.h"

std::string substrr(std::string const& str, size_t beg, size_t end)
{
	if (end < beg)
		throw std::range_error("");
	if (end != std::string::npos) {
		return str.substr(beg, end - beg);
	}
	return str.substr(beg, end);
}

std::string findAndReplace(const std::string& source, const std::string& find, const std::string& replace)
{
	std::string result = source;
    std::string::size_type fLen = find.size();
    std::string::size_type rLen = replace.size();
    for (std::string::size_type pos = 0; (pos = result.find(find, pos)) != std::string::npos; pos += rLen)
    {
        result.replace(pos, fLen, replace);
    }
    return result;
}