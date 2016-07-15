#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct {
	char* MAddr;
	char* MSrvc;
	char* UAddr;
	char* USrvc;
	int debug;
} Args;

// Oversimplified function for parsing urls (modifies url!!!)
int parseUrl( char* url, char** addr, char** port )
{
	// Port is after last semicolon?
	*port = strrchr( url, ':' );
	if ( *port == NULL )
		return -1;
	*port += 1;
	if ( *(*port - 2) == ']' ) {
		*(*port - 2) = '\0';
		*addr = url + 1;
	} else {
		*(*port - 1) = '\0';
		*addr = url;
	}
	return 0;
}

// Signal handler
int shouldExit = 0;
void signal_handler ( int signum )
{
	shouldExit = 1;
}

const char *helpMsg = "\
m2u - mutlicast to unicast proxy\n"
"author: Jan Spisiak (xspisi03@stud.fit.vutbr.cz)\n"
"usage: m2u -h MHOST -g UHOST\n"
"		m2u -?\n";

#define DATA_BUF_SIZE 2048

int main( int argc, char *argv[] )
{
	int ret = 0;
	Args args;

	int msd, usd;
	struct addrinfo *mhost, *uhost;

	// Process arguments
	{
		char opt;
		while ( ( opt = getopt( argc, argv, "h:g:d?" )) != -1 ) {
			switch (opt) {
				case 'h':
					if (parseUrl(optarg, &args.MAddr, &args.MSrvc) < 0) {
						fprintf(stderr,"No port specified in multicast host\n");
						return -1;
					}
					break;
				case 'g':
					if (parseUrl(optarg, &args.UAddr, &args.USrvc) < 0) {
						fprintf(stderr,"No port specified in unicast host\n");
						return -1;
					}
					break;
				case 'd':
					args.debug = 1;
					break;
				case '?':
					fprintf(stderr,"%s",helpMsg);
					return 0;
					break;
				default:
					fprintf(stderr, "Unknown parameter %c\n", opt);
					break;
			}
		}
	}

	// Setup signal_handler
	{
		struct sigaction new_action, old_action;

		new_action.sa_handler = signal_handler;
		sigemptyset (&new_action.sa_mask);
		new_action.sa_flags = 0;

		sigaction (SIGINT, NULL, &old_action);
		if (old_action.sa_handler != SIG_IGN)
			sigaction (SIGINT, &new_action, NULL);
		sigaction (SIGHUP, NULL, &old_action);
		if (old_action.sa_handler != SIG_IGN)
			sigaction (SIGHUP, &new_action, NULL);
		sigaction (SIGTERM, NULL, &old_action);
		if (old_action.sa_handler != SIG_IGN)
			sigaction (SIGTERM, &new_action, NULL);
	}

	// Resolve hostname and service
	{
		fprintf(stderr, "Resolving hostnames.." );
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;

		if ( getaddrinfo( args.MAddr, args.MSrvc, &hints, &mhost ) != 0 ) {
			perror( "error" );
			ret = 1; goto end;
		}
		if ( getaddrinfo( args.UAddr, args.USrvc, &hints, &uhost ) != 0 ) {
			perror( "error" );
			ret = 1; goto mhost;
		}
		fprintf(stderr, "OK\n" );
	}

	// Create sockets
	{
		fprintf(stderr, "Opening sockets.." );
		msd = socket( mhost->ai_family, SOCK_DGRAM, 0 );

		if ( msd < 0 ) {
			perror( "Error opening multicast socket " );
			ret = 1; goto uhost;
		}
		usd = socket( uhost->ai_family, SOCK_DGRAM, 0 );
		if ( usd < 0 ) {
			perror( "Error opening unicast socket " );
			ret = 1; goto msdc;
		}
		fprintf(stderr, "OK\n" );
	}

	// Allow reuse of sockets
   {
		fprintf(stderr, "Setting SO_REUSEADDR.. " );
		int reuse = 1;

		if ( setsockopt( msd, SOL_SOCKET, SO_REUSEADDR, ( char * ) &reuse, sizeof( reuse ) ) < 0 ) {
			perror( "error" );
			ret = 1; goto sdc;
		}
		if ( setsockopt( usd, SOL_SOCKET, SO_REUSEADDR, ( char * ) &reuse, sizeof( reuse ) ) < 0 ) {
			perror( "error" );
			ret = 1; goto sdc;
		}
		fprintf(stderr, "OK\n" );
	}

	// "Connect" on unicast socket
	{
		fprintf(stderr, "Connecting unicast socket.." );
		if ( connect( usd, uhost->ai_addr, uhost->ai_addrlen ) ) {
			perror( "error" );
			ret = 1; goto sdc;
		} else
			fprintf(stderr, "OK\n" );
	}

	struct ip_mreqn mgroup;
	struct ipv6_mreq mgroup6;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr *saddr;
	int level, optname;
	void *optval;
	socklen_t optlen;
	// Prepare structures for bind and setsockopt multicast option
	{
		if ( mhost->ai_family == AF_INET ) {
			sin = *(struct sockaddr_in *)mhost->ai_addr;
			saddr = (struct sockaddr *)&sin;

			level = IPPROTO_IP;
			optname = IP_ADD_MEMBERSHIP;
			optval = &mgroup;
			optlen = sizeof( mgroup );
			// Address is given, interface and local address can be any
			mgroup.imr_multiaddr = sin.sin_addr;
			mgroup.imr_address.s_addr = INADDR_ANY;
			//mgroup.imr_address = sin.sin_addr;
			mgroup.imr_ifindex = 0;
		} else {
			sin6 = *(struct sockaddr_in6 *)mhost->ai_addr;
			saddr = (struct sockaddr *)&sin6;

			level = IPPROTO_IPV6;
			optname = IPV6_JOIN_GROUP;
			optval = &mgroup6;
			optlen = sizeof( mgroup6 );
			// Address is given, interface and local address can be any
			mgroup6.ipv6mr_multiaddr = sin6.sin6_addr;
			mgroup6.ipv6mr_interface = 0;
		}
		saddr->sa_family = mhost->ai_family;
	}

	// Bind multicast socket to port
	{
		fprintf(stderr, "Binding local multicast socket.." );
		if ( bind( msd, saddr, mhost->ai_addrlen ) ) {
			perror( "error" );
			ret = 1; goto sdc;
		} else
			fprintf(stderr, "OK\n" );
	}

	// Setsockopt for multicast
	{
		fprintf(stderr, "Adding multicast group.." );
		if ( setsockopt( msd, level, optname, optval, optlen ) < 0 ) {
			perror( "error" );
			ret = 1; goto sdc;
		} else
			fprintf(stderr, "OK\n" );
	}

	// Read and forward
	{
		fprintf(stderr, "Reading datagram messages..\n" );
		int datalen;
		char databuf[DATA_BUF_SIZE];
		datalen = sizeof( databuf );
		// Remember how many packets we had no error
		int noError = 14;
		while ( !shouldExit ) {
			int recvr = recv( msd, databuf, datalen, 0 );
			int sendr;
			if ( recvr < 0 ) {
				perror( "Reading datagram message error" );
				ret = 1; break;
			} else {
				int sent = 0;
				// Make sure to send whole buffer
				while ( sent < recvr && ( sendr = send( usd, databuf + sent, recvr - sent, 0 ) ) > 0) {
					sent += sendr;
				}

				if ( sendr < 0 ) {
					if ( errno != ECONNREFUSED ) {
						perror( "Send() error" );
						//ret = 1; break;
					} else if ( noError > 1 ) {
						fprintf(stderr, "Unicast host doesn't listen.\n" );
					}
					noError = 0;
				} else {
					if ( noError < 16 ) {
						if ( noError == 15 )
							fprintf(stderr, "Connection seems to be OK.\n" );
						noError++;
					}
				}
			}
		}
	}

	fprintf(stderr, "Caught signal, cleaning up..");
	// Clean up
sdc:
	close(usd);
msdc:
	close(msd);
uhost:
	freeaddrinfo(uhost);
mhost:
	freeaddrinfo(mhost);

end:
	return ret;
}
