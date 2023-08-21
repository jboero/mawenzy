#include <iostream>
#include <fstream>
#include <exception>
#include <sys/stat.h>
#include "StdColors.h"

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int sockets();

// Returns socketfd.
int sockets()
{
	int sockfd, portno = 8888, n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;
	struct hostent *server = gethostbyname("localhost");

	if (sockfd < 0)
	{
		cerr << RED << "ERROR opening socket" << RESET << endl;
		return -1;
	}

	if (!server)
	{
		cerr << RED << "ERROR, no such host" << RESET << endl;
		return -2;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);

	serv_addr.sin_port = htons(portno);

	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		return -3;
	
	return sockfd;
}

int main(int argc, char *argv[])
{
	bool debug = false;
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	const int buffsize = 2048;
	char rbuffer[buffsize];
	string buffer;

	char *cvalue = NULL;
	int index;
	char c;

	while ((c = getopt(argc, argv, "xy")) != -1)
	{
		switch (c)
		{
		  case 'x':
			debug = true;
			break;
		  case 'h':
			cout << CYAN << "HELP" << RESET << endl;
			return 0;
		  default:
			cout << CYAN << "USAGE" << RESET << endl;
			break;
		}
	}

	#if DEBUG
	if (debug)
		for (int i = 0; i < argc; ++i)
			cout << '$' << i << ": " << argv[i] << endl;
	#endif

	if ((sockfd = sockets()) < 0)
	{
		cerr << "Socket error: " << sockfd << endl;
		return 1;
	}

	if (optind < argc)	// Script arg?
	{
		try
		{
			ifstream script(argv[optind++]);
			string header;

			if (getline(script, header))
			{
				char rbuffer[buffsize];
				int n = write(sockfd, header.c_str(), header.length());

				if (n < 0)
				{
					cerr << RED << "ERROR writing to socket" << RESET << endl;
					return 1;
				}
/*				bzero(rbuffer, buffsize);
				while ((n = read(sockfd, rbuffer, buffsize)) > 0)
				{
					// TODO answer logic
					cout << CYAN << buffer << RESET;
				}
*/
				cout << "OK" << endl;
				if (n < 0)
					 cerr << RED << "ERROR reading from socket" << RESET << endl;

				close(sockfd);
			}
		}
		catch (exception e)
		{
			cerr << RED << "Problem, yo: " << e.what() << RESET << endl;
			return 1;
		}
	}
	else	// Interactive mode.
	{
		while (getline(cin, buffer))
		{
			// TODO
		}
	}

	//std::replace( s.begin(), s.end(), 'x', 'y');
	return 0;
}
