#ifndef CLIENT_H
#define CLIENT_H

#include <exception>
#include <stdexcept>
#include <string>
#include <fstream>
#include <cstring>
#include <cassert>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <errno.h>

#include "Slog.h"
#include "Connection.h"

class Client {
		Connection con;
	public:
		Client();
		~Client();

		void connect(const std::string& host, const std::string& service);
		void disconnect();

		void saveFile(const std::string& file);
};

#endif