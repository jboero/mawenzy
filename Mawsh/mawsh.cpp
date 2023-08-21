/* 
 * This file is part of the Mawenzy Client (https://github.com/jboero/mawenzy).
 * Copyright (c) 2015 John Boero.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <sstream>
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
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

int sockets();

// Returns socketfd.
int sockets()
{
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un serv_addr;
	const char *sockpath = getenv("MAWD_SOCK");

	if (sockfd < 0)
	{
		cerr << RED << "ERROR creating socket" << RESET << endl;
		return -1;
	}

	memset(&serv_addr, 0, sizeof(struct sockaddr_un));

	serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path, sockpath ? sockpath : "/tmp/mawd.sock", sizeof(serv_addr.sun_path) - 1);

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
		return -3;
	
	return sockfd;
}

int main(int argc, char *argv[])
{
	bool debug = false;
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	const int buffsize = 4096;
	char rbuffer[buffsize];
	string buffer, response;
	stringstream rbuff;

	char *cvalue = NULL;
	int index;
	char c;

	while ((c = getopt(argc, argv, "d")) != -1)
	{
		switch (c)
		{
		  case 'd':
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

	if (optind < argc)	// Stored script arg?
	{
		try
		{
			const char *csc = argv[optind++];
			ifstream script(csc);
			struct stat st_buff;
			time_t mtime = 0;
			string header, body;
			stringstream sheader;

			if (getline(script, header))
			{
				// Script identifier "inode|mtime"
				if (!stat(csc, &st_buff))
					sheader << st_buff.st_ino << '|'<< st_buff.st_mtime << ' ';
				
				sheader << header;
				for (int i = 0; i < argc; ++i)
					sheader << ' ' << argv[i];

				string response(sheader.str());
				int wsize = write(sockfd, response.c_str(), response.length());
				if (wsize < 0)
				{
					cerr << RED << "ERROR writing to socket" << RESET << endl;
					return 1;
				}

				// Read response
				bzero(rbuffer, buffsize);
				wsize = buffsize;
				while (wsize == buffsize)
				{
					wsize = read(sockfd, rbuffer, buffsize);
					if (wsize < 0)
					{
						cerr << RED << "ERROR Socket read failed." << RESET << endl;
						break;
					}
					
					rbuff.write(rbuffer, wsize);
				}

				response = rbuff.str();
				if (response == "NEWSTOME")
				{
					//cout << YELLOW << "NEWSTOME. BEGIN send script." << RESET << endl;

					script.seekg(0, ios::end);
					int length = script.tellg();
					int hlen = header.length() + 1;
					int blen = length - hlen;

					script.seekg(hlen);
					body.reserve(blen);
					body.assign(istreambuf_iterator<char>(script), istreambuf_iterator<char>());

					wsize = write(sockfd, body.c_str(), body.length());
				}
				response = "";

				// Receive response and cout
				do
				{
					bzero(rbuffer, buffsize);
					wsize = read(sockfd, rbuffer, buffsize);
					if (wsize < 0)
					{
						cerr << RED << "ERROR Socket read failed." << RESET << endl;
						break;
					}

					cout << rbuffer;
				} while (wsize == buffsize);

				if (wsize < 0)
					 cerr << RED << "ERROR reading from socket" << RESET << endl;
				close(sockfd);
				//cout << endl;
			}
		}
		catch (exception e)
		{
			cerr << RED << "Mawsh problem (client): " << e.what() << RESET << endl;
			close(sockfd);
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

	return 0;
}
