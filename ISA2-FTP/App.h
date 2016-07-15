#ifndef APP_H
#define APP_H

#include <exception>
#include <stdexcept>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sstream>

#include "FtpClient.h"

using namespace std;

class App {
		
	public:
		void parseUrl(string const &url, string &user, string &password, string &host, string &port, string &path);
		int run(int argc, const char* argv[]);
};

#endif