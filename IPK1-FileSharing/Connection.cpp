#include "Connection.h"

using namespace std;

Connection::Connection(int socket) : socket(socket) {
	buffer.reserve(Message::headerLength);
}

void Connection::readMessage(Message& msg) {
	ssize_t readStat = 1;

	readStat = ::recv(socket, &buffer[0], Message::headerLength, MSG_WAITALL);

	if (readStat < 0) {
		throw runtime_error(string("Couldn't recv on socket: ") + strerror(errno));
	} else if (readStat == 0) {
		throw runtime_error("Remote unexpectedly closed connection.");
	}

	msg.type = static_cast<Message::Type>(buffer[0]);
	msg.length = ntohs(*(uint16_t *)(&buffer[1]));
	msg.data.clear();
	msg.data.reserve(msg.length);

	size_t readCount = 0;
	while (readCount != msg.length) {
		// TCP will preserve data in the stream if more than specified len
		buffer.resize(msg.length - readCount);
		readStat = ::recv(socket, &buffer[0], msg.length - readCount, 0);

		if (readStat < 0) {
			throw runtime_error(string("Couldn't recv on socket: ") + strerror(errno));
		} else if (readStat == 0) {
			throw runtime_error("Remote unexpectedly closed connection.");
		}

		buffer.resize(readStat);
		msg.data.insert(msg.data.end(), buffer.begin(), buffer.end());
		readCount += readStat;
	}
}

void Connection::sendMessage(const Message& msg) {
	ssize_t rSend = 0;
	size_t len = Message::headerLength;
	const uint8_t *buff = &buffer[0];
	buffer[0] = static_cast<uint8_t>(msg.type);
	*(uint16_t*)&buffer[1] = htons(msg.length);

	while (len > 0) {
		rSend = ::send(socket, buff, len, MSG_NOSIGNAL);
		if (rSend == -1) {
			throw runtime_error(string("Couldn't send on socket: ") + strerror(errno));
		}
		buff += rSend;
		len -= rSend;
	}

	buff = static_cast<const uint8_t *>(&msg.data[0]);
	len = msg.length;
	while (len > 0) {
		rSend = ::send(socket, buff, len, MSG_NOSIGNAL);
		if (rSend == -1) {
			throw runtime_error(string("Couldn't send on socket: ") + strerror(errno));
		}
		buff += rSend;
		len -= rSend;
	}
}

int Connection::isConnected() {
	if (socket >= 0)
		return 1;
	return 0;
}

void Connection::close() {
	if (socket >= 0) {
		::close(socket);
		socket = -1;
	}
}