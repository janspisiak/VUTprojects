#include "Server.h"

using namespace std;

Server::Server() : lSocket(-1) {

}

void Server::bind(const std::string& service) {
	int status;
	struct addrinfo hints,
		*servinfo,
		*remote;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = ::getaddrinfo(NULL, service.c_str(), &hints, &servinfo)) != 0) {
		throw runtime_error(gai_strerror(status));
	}

	for(remote = servinfo; remote != NULL; remote = remote->ai_next) {
		// Open socket
		lSocket = ::socket(remote->ai_family, remote->ai_socktype, remote->ai_protocol);
		if (lSocket == -1) {
			Slog(Debug) << "Couldn't create socket: " << strerror(errno) << ". Trying next..\n";
			continue;
		}

		/*if (setsockopt(lSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}*/

		status = ::bind(lSocket, remote->ai_addr, remote->ai_addrlen);
		if (status == -1) {
			::close(lSocket);
			Slog(Debug) << "Couldn't bind on socket: " << strerror(errno) << ". Trying next..\n";
			continue;
		}

		break;
	}

	if (remote == NULL) {
		if (lSocket == -1) {
			throw runtime_error(string("Couldn't create socket: ") + strerror(errno));
		} else if (status == -1) {
			throw runtime_error(string("Couldn't bind on socket: ") + strerror(errno));
		} else {
			throw runtime_error("Couldn't connect to server: Unknown error.");
		}
	}

	Slog(Debug) << "Binded on service: " << service << "\n";

	freeaddrinfo(servinfo);
}

void Server::run(int maxSpeed) {
	int running = 1;
	int newSocket = -1;
	struct sockaddr_in remoteAddr; // connector's address information
	char remoteAddrStr[INET6_ADDRSTRLEN];
	socklen_t sockLen = sizeof(struct sockaddr_in);

	if (listen(lSocket, MAX_QUEUE_LISTEN) == -1) {
		throw runtime_error(string("Couldn't listen on socket: ") + strerror(errno));
	}

	while (running) {
		newSocket = ::accept(lSocket, (struct sockaddr *)&remoteAddr, &sockLen);
		if (newSocket == -1) {
			throw runtime_error(string("Couldn't accept: ") + strerror(errno));
		}

		::inet_ntop(remoteAddr.sin_family, &(remoteAddr.sin_addr), remoteAddrStr, sizeof(remoteAddrStr));
		Slog(Debug) << "Accepted connection from client " << newSocket << " at: " << remoteAddrStr << "\n";

		std::thread th(resolveConnection, newSocket, maxSpeed * 1000);
		th.detach();
	}
}

void resolveConnection(int socket, uint32_t maxBytesUp) {
	try {
		Connection con(socket);

		Message msg;
		while (1) {
			con.readMessage(msg);

			switch (msg.type) {
				case Message::Type::Error: {
					Slog(Warning) << "Client " << socket << ": " << (char *)&msg.data[0];
					break;
				} case Message::Type::Quit: {
					Slog(Debug) << "Client " << socket << " disconnected.";
					con.close();
					return;
					break;
				} case Message::Type::GetFile: {
					string file(msg.data.begin(), msg.data.end());
					Slog(Debug) << "Client " << socket << ": getFile: " << file;
					ifstream ifs;
					uint32_t length;
					uint32_t sendCount = 0;

					ifs.exceptions( ifstream::failbit | ifstream::badbit );
					try {
						ifs.open(file, ios::in | ios::binary | ios::ate);

						length = ifs.tellg();
						msg.type = Message::Type::File;
						msg.data.resize(sizeof length);
						*(uint32_t *)&msg.data[0] = length;
						msg.length = msg.data.size();
						con.sendMessage(msg);
						Slog(Debug) << "Thread " << socket << ": Opened file \"" << file << "\" with size: " << length;

						ifs.seekg(0, ios::beg);
						msg.type = Message::Type::Data;

						std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
						std::chrono::milliseconds elapsedMilliS, sleepFor;
						uint32_t sentThisSec = 0;

						Slog(Debug) << "Thread " << socket << ": " << "Sending file with speed " << maxBytesUp / 1000 << "kB/s";
						start = std::chrono::high_resolution_clock::now();
						while (sendCount != length) {
							if (sentThisSec >= maxBytesUp) {
								end = std::chrono::high_resolution_clock::now();
								elapsedMilliS = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
								sleepFor = std::chrono::milliseconds(1000) - elapsedMilliS;

								std::this_thread::sleep_for(sleepFor);

								start = std::chrono::high_resolution_clock::now();
								sentThisSec = 0;
							}

							uint32_t readSize = min((uint32_t)maxBytesUp, min((uint32_t)Message::maxLength, length - sendCount));
							msg.data.resize(readSize);
							ifs.read((char *)msg.data.data(), readSize);
							msg.length = msg.data.size();
							con.sendMessage(msg);

							sendCount += readSize;
							sentThisSec += readSize;
						}
					} catch (const std::ios_base::failure& e) {
						msg.type = Message::Type::Error;
						string err("Problem reading file \"" + file + "\": " + e.what());
						msg.data.assign(err.begin(), err.end());
						msg.length = msg.data.size();
						con.sendMessage(msg);

						Slog(Warning) << "Thread " << socket << ": " << err;
					}
					break;
				} default: {
					Slog(Warning) << "Client sent unexpected Message.";
					break;
				}
			}
		}

	} catch (const std::exception& e) {
		Slog(Error) << "Client " << socket << ": " << e.what();
	}
	Slog(Debug) << "Thread " << socket << " end.";
}