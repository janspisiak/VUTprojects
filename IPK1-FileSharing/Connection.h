#ifndef CONNECTION_H
#define CONNECTION_H

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
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

class Message {
	public:
		enum class Type : std::uint8_t {
			Data = 0,
			Quit,
			Error,
			GetFile,
			File
		};

		Type type;
		std::uint16_t length;
		static const size_t headerLength = sizeof(type) + sizeof(length);
		static const uint16_t maxLength = UINT16_MAX;

		std::vector<std::uint8_t> data;
};

class Connection {
		int socket;

		std::vector<std::uint8_t> buffer;
	public:
		Connection(int socket);

		int isConnected();
		void readMessage(Message& msg);
		void sendMessage(const Message& msg);
		void close();
};

#endif