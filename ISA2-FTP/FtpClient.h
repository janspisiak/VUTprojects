#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <errno.h>

#include "Utilities.h"

using namespace std;

class FtpClient {
		struct sockaddr remoteAddress;
		size_t remoteAddressLength;
		int controlSocket;
		int dataSocket;

		char* recvBuffer;
		size_t recvBufferSize;

		string responseBuffer;
	public:
		FtpClient();
		~FtpClient();
		void openCon(string const & address, string const & service, string const & user, string const & password);
		void openDataCon(unsigned short port);
		void cwd(string const & path);
		void list(string &list, string const & options);
		void epsv(unsigned short &port);
		void quit();
		void close();
		void list(string const & path);

		void send(string const & data);
		ssize_t read(string &data);
		ssize_t readData(string &data);
		string readResponse();
		void checkResponse(string const & response, string validStatus);
};

#endif