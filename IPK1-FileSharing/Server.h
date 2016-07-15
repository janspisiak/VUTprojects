#ifndef SERVER_H
#define SERVER_H

#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ratio>
#include <cstring>
#include <cassert>
#include <cstdint>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <errno.h>

#include "Slog.h"
#include "Connection.h"

#define MAX_QUEUE_LISTEN 10

class Server {
		int lSocket;
	public:
		Server();

		void bind(const std::string& service);
		void run(int maxSpeed);
};

void resolveConnection(int socket, uint32_t maxBytesDown);

#endif