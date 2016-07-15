#include "App.h"

int App::run(int argc, const char* argv[]) {
	int returnCode = 0;
	try {
		if (argc < 2) {
			throw logic_error("Host not specified. Check --help\n");
		}

		if (argc > 2) {
			throw logic_error("Too many arguments.\n");
		}

		string user, password, host, port, path;

		if (strcmp(argv[1], "--help") == 0) {
			std::cout << "Some help message.Multiple lines, lol.";
			return 0;
		} else {
			string url(argv[1]);
			parseUrl(url, user, password, host, port, path);
		}

		Logger::verbosity = Logger::Severity::Debug;

		Slog(Debug) << "User: \"" << user << "\"";
		Slog(Debug) << "Password: \"" << password << "\"";
		Slog(Debug) << "Host: \"" << host << "\"";
		Slog(Debug) << "Port: \"" << port << "\"";
		Slog(Debug) << "Path: \"" << path << "\"";

		if (host.empty()) {
			throw logic_error("No host name specified.");
		}
		if (port.empty())
			port = "21";
		if (user.empty() && password.empty()) {
			user = "anonymous";
		}

		auto ftpClient = new FtpClient();
		ftpClient->openCon(host, port, user, password);

		string command, response;

		if (!path.empty())
			ftpClient->cwd(path);

		string list;
		ftpClient->list(list, "");
		std::cout << list;

		ftpClient->quit();

		delete ftpClient;
	} catch (const std::exception& e) {
		Slog(Error) << e.what();
		returnCode = 1;
	}
	return returnCode;
}

void App::parseUrl(string const &url, string &user, string &password, string &host, string &port, string &path)
{
	size_t lastParsed = 0;
	if (url.compare(0, 6, "ftp://") == 0) {
		lastParsed = 6;
		size_t loginDelim = url.find('@', lastParsed);
		if (loginDelim != string::npos) {
			size_t userDelim = url.find(':', lastParsed);
			if ((loginDelim != string::npos) && (userDelim < loginDelim)) {
				user = substrr(url, lastParsed, userDelim);
				password = substrr(url, userDelim + 1, loginDelim);
				lastParsed = loginDelim + 1;
			} else {
				std::cerr << "Can't parse user/password from url.\n";
			}
		}
	}

	size_t portDelim = url.find(':', lastParsed);
	size_t pathDelim = url.find('/', lastParsed);
	if ((portDelim != string::npos) && ((pathDelim == string::npos) || (pathDelim > portDelim))) {
		host = substrr(url, lastParsed, portDelim);
		port = substrr(url, portDelim + 1, pathDelim);
	} else {
		host = substrr(url, lastParsed, pathDelim);
	}

	if (pathDelim != string::npos) {
		path = substrr(url, pathDelim, string::npos);
	}
}