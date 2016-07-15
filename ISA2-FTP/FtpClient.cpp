#include "FtpClient.h"

void FtpClient::send(string const & data)
{
	ssize_t rSend = 0;
	size_t len = data.length();
	const char* buff = data.c_str();

	Slog(Debug) << "OUT:\t" << findAndReplace(data, "\r\n", "\\r\\n");

	while (len > 0) {
		rSend = ::send(controlSocket, buff, len, 0);
		if (rSend == -1) {
			throw runtime_error("Couldn't send.");
		}
		buff += rSend;
		len -= rSend;
	}
}

ssize_t FtpClient::readData(string &data)
{

	ssize_t readCount = 1;

	while (readCount != 0) {
		readCount = ::recv(dataSocket, recvBuffer, recvBufferSize, 0);
		if (readCount > 0) {
			data.append(recvBuffer, readCount);
		} else if (readCount < 0) {
			throw runtime_error("Couldn't read from socket.");
		}
	}

	::close(dataSocket);
	dataSocket = -1;

	return readCount;
}

string FtpClient::readResponse()
{
	// stupid method, it expects continous message
	size_t found = 0;
	size_t lastFound;
	size_t lastPos = 0;
	ssize_t readCount = 0;

	string status;
	do {
		lastFound = found;

		found = responseBuffer.find("\r\n", lastPos);
		while (found == string::npos) {
			readCount = ::recv(controlSocket, recvBuffer, recvBufferSize, 0);
			if (readCount > 0) {
				responseBuffer.append(recvBuffer, readCount);
				found = responseBuffer.find("\r\n", lastPos);
			} else {
				throw runtime_error("Couldn't read from socket.");
			}
		}
		lastPos = found + 1;
		found += 2;
		status = responseBuffer.substr(lastFound, 3);
	} while ((responseBuffer[lastFound + 3] == '-') || (!std::all_of(status.begin(), status.end(), ::isdigit)));

 	string response = responseBuffer.substr(0, found);
 	responseBuffer.erase(0, found);

	Slog(Debug) << " IN:\t" << findAndReplace(response, "\r\n", "\n\t");

	return response;
}

void FtpClient::checkResponse(string const & response, string validStatus)
{
	// Very bad simple checking
	if (response.compare(0, 3, validStatus) != 0) {
		throw runtime_error("Received invalid status: " + response.substr(0, 3) + " " + validStatus);
	}
}

void FtpClient::openDataCon(unsigned short port)
{
	// Absolutely wrong, but works for now
	struct sockaddr_in* servaddr = (struct sockaddr_in*)&remoteAddress;
	servaddr->sin_port = htons(port);
	// Connect to remote server
	if (::connect(dataSocket, (struct sockaddr*)servaddr, remoteAddressLength) < 0) {
		throw runtime_error(string("Couldn't connect to server: ") + strerror(errno));
	}
}

void FtpClient::openCon(string const & address, string const & service, string const & user, string const & password)
{
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(address.c_str(), service.c_str(), &hints, &servinfo)) != 0) {
	    throw runtime_error(gai_strerror(status));
	}

	// Create control socket
	controlSocket = ::socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (controlSocket == -1) {
		throw runtime_error("Couldn't create socket.");
	}

	// Create data socket
	dataSocket = ::socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (dataSocket == -1) {
		throw runtime_error("Couldn't create socket.");
	}

	// Connect to remote server
	if (::connect(controlSocket, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		throw runtime_error(string("Couldn't connect to server: ") + strerror(errno));
	}

	remoteAddress = *(servinfo->ai_addr);
	remoteAddressLength = servinfo->ai_addrlen;

	freeaddrinfo(servinfo);

	string response = readResponse();
	checkResponse(response, "220");

	send("USER " + user + "\r\n");
	response = readResponse();
	checkResponse(response, "331");

	send("PASS " + password + "\r\n");
	response = readResponse();
	checkResponse(response, "230");
}

void FtpClient::cwd(string const & path)
{
	send("CWD " + path + "\r\n");
	string response = readResponse();
	checkResponse(response, "250");
}

void FtpClient::list(string &list, string const & options = "")
{
	unsigned short port;
	epsv(port);
	openDataCon(port);
	send("LIST " + options + "\r\n");
	string response = readResponse();
	checkResponse(response, "150");
	readData(list);
	response = readResponse();
	checkResponse(response, "226");
}

void FtpClient::epsv(unsigned short &port)
{
	send("EPSV\r\n");
	string response = readResponse();
	checkResponse(response, "229");

	size_t addrDelim = response.find('(');
	size_t addrEndDelim = response.find(')');
	string sPort = substrr(response, addrDelim + 4, addrEndDelim - 1);
	Slog(Debug) << "EPSV port:" << sPort;
	unsigned long lPort = ::strtoul(sPort.c_str(), NULL, 0);
	port = lPort;
}

void FtpClient::quit()
{
	send("QUIT\r\n");
	string response = readResponse();
	checkResponse(response, "221");
}

void FtpClient::close()
{
	if (controlSocket > 0) {
		::close(controlSocket);
		controlSocket = -1;
	}
	if (dataSocket > 0) {
		::close(dataSocket);
		dataSocket = -1;
	}
}

FtpClient::FtpClient()
{
	controlSocket = -1;
	dataSocket = -1;

	recvBufferSize = 2048;
	recvBuffer = new char[recvBufferSize];
}

FtpClient::~FtpClient()
{
	this->close();

	delete [] recvBuffer;
}