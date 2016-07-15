#include "Client.h"

using namespace std;

Client::Client() : con(-1) {

}

void Client::connect(const std::string& host, const std::string& service) {
	int cSocket = -1;
	int status;
	struct addrinfo hints,
		*servinfo,
		*remote;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(host.c_str(), service.c_str(), &hints, &servinfo)) != 0) {
		throw runtime_error(gai_strerror(status));
	}

	for(remote = servinfo; remote != NULL; remote = remote->ai_next) {
		// Open socket
		cSocket = ::socket(remote->ai_family, remote->ai_socktype, remote->ai_protocol);
		if (cSocket == -1) {
			Slog(Debug) << "Couldn't create socket: " << strerror(errno) << ". Trying next..\n";
			continue;
		}

		// Connect to remote server
		status = ::connect(cSocket, remote->ai_addr, remote->ai_addrlen);
		if (status == -1) {
			close(cSocket);
			Slog(Debug) << "Couldn't connect on socket: " << strerror(errno) << ". Trying next..\n";
			continue;
		}

		break;
	}

	if (remote == NULL) {
		if (cSocket == -1) {
			throw runtime_error(string("Couldn't create socket: ") + strerror(errno));
		} else if (status == -1) {
			throw runtime_error(string("Couldn't connect on socket: ") + strerror(errno));
		} else {
			throw runtime_error("Couldn't connect to server: Unknown error.");
		}
	}

	char remoteAddr[INET6_ADDRSTRLEN];
	inet_ntop(remote->ai_family, &((struct sockaddr_in*)remote->ai_addr)->sin_addr, remoteAddr, sizeof remoteAddr);
	Slog(Debug) << "Connected to: " << remoteAddr << "\n";

	con = Connection(cSocket);

	freeaddrinfo(servinfo);
}

void Client::disconnect() {
	if (con.isConnected()) {
		Message msg;
		msg.type = Message::Type::Quit;
		msg.length = 0;
		con.sendMessage(msg);
		con.close();
	}
}

Client::~Client() {
	disconnect();
}

void Client::saveFile(const std::string& file) {
	Message msg;
	ofstream ofs;

	Slog(Debug) << "Requesting file: " << file;
	msg.type = Message::Type::GetFile;
	msg.data.assign(file.begin(), file.end());
	msg.length = msg.data.size();
	con.sendMessage(msg);

	con.readMessage(msg);
	if (msg.type == Message::Type::Error)
		throw runtime_error(string("Server error: ") + string(msg.data.begin(), msg.data.end()));
	else if (msg.type != Message::Type::File)
		throw runtime_error("Unexpected message from server.");

	ofs.exceptions( ofstream::failbit | ofstream::badbit );
	ofs.open(file, ios::out | ios::trunc | ios::binary);

	//uint32_t fileLength = ::ntohl(*(uint32_t *)(&msg.data[0]));
	uint32_t fileLength = *(uint32_t *)(&msg.data[0]);
	Slog(Debug) << "Downloading file \"" << file << "\" with size: " << fileLength;
	uint64_t readCount = 0;
	while (readCount != fileLength) {
		con.readMessage(msg);
		if (msg.type == Message::Type::Error)
			throw runtime_error(string("Server error: ") + string(msg.data.begin(), msg.data.end()));
		else if (msg.type != Message::Type::Data)
			throw runtime_error("Unexpected message from server.");
		ofs.write((const char *)msg.data.data(), msg.data.size());
		readCount += msg.data.size();
	}

	ofs.close();
}