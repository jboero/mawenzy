/* 
 * This file is part of the Mawenzy Server (https://github.com/jboero/mawenzy).
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
//#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/syscall.h>
#include <chrono>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <map>
#include <sstream>

#include "Sockets.h"
#include "MawNodes/MawNode.h"
#include "MawNodes/ProcNode.h"
#include "Mawd.h"
#include "StdColors.h"

using namespace std;

typedef struct margs
{
	char *argv[10];
	int argc;
} margs;

long getNanoNow()
{
	timespec  time;
	clock_gettime(CLOCK_REALTIME, &time);
	return time.tv_nsec;
}

int getMilliNow()
{
	return getNanoNow() * 1000;
}

long getSpan(long nTimeStart)
{
	long nSpan = getMilliNow() - nTimeStart;
	if(nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}

void printNanoSince()
{
	static long last;
	cout << CYAN << dec << getSpan(last) << "ns" << RESET << endl;
	last = getNanoNow();
}

void* connection_handler(void *pair)
{
	if (!pair)
	{
		cerr << "ERR null parameter in connection_handler." << endl;
		return NULL;
	}

	const int buffsize = 0x1000;
	int res;
	char rbuffer[buffsize];
	map<string, string> paramsRaw;
	map<string, GPUNode*> paramsBuffer;
	vector<string> kernNames;
	struct socket_pair *spair = (socket_pair*)pair;
	int k, wsize, rsize = buffsize, sock = spair->socket;
	string reply, script, arg;
	stringstream header, body;
	cl::Program *prog = NULL;
	cl::Event timeEvents[10];

	try
	{
		// Get our thread ID
		int tid = syscall(SYS_gettid);
		string dirname(to_string(tid));
		ProcNode procNode(tid);

		paramsRaw["p"] = "p";
		paramsBuffer["p"] = &(procNode.buf);

		#if DEBUG
		cout << "Creating proc node " << tid << endl;
		#endif

		spair->state->proc.Add(dirname, &procNode);

		while (rsize == buffsize)
		{
			rsize = read(sock, rbuffer, buffsize);
			#if DEBUG
			cout << "header read (" << rsize << "): " << rbuffer << endl;
			#endif
			header.write(rbuffer, rsize);
		}

		// Get identifier "inode|mtime"
		header >> script;

		cl::Context context = cl::Context::getDefault();
		cl::Device device = cl::Device::getDefault();

		// Do we already have this script?
		if (!(prog = spair->state->knowns[script]))
		{
			// Reply that we don't have this one cached.
			reply = "NEWSTOME";

			#if DEBUG
			cout << "New program incoming:" << endl;
			#endif

			wsize = write(sock, reply.c_str(), reply.length());

			// Receive script from client.
			rsize = buffsize;
			while (rsize == buffsize)
			{
				rsize = recv(sock, rbuffer, buffsize, 0);
				body.write(rbuffer, rsize);
			}

			cl::Program::Sources sources(spair->state->common_src);
			string source = body.str();
			const char *csource = source.c_str();

			#if DEBUG
			cout << source << endl;
			#endif

			sources.push_back({csource, source.length()});
			prog = new cl::Program(context, sources);

			try
			{
				// Note need to add -cl-kernel-arg-info for arg info (duh?)
				// TODO use createKernels(vector<Kernel>* kernels) to fetch and store all kernels..
				prog->build({device}, " -cl-kernel-arg-info -I /usr/include/mawsh/ ");
				spair->state->knowns[script] = prog;

				// TODO check for earlier version (inode|ANYMTIME) and purge it.
			}
			catch (cl::Error e)
			{
				reply = string(RED) + "ERROR: " + prog->getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl::Device::getDefault()) + RESET;
				write(sock, reply.c_str(), reply.length());
				close(sock);

				cerr << reply << endl;
				delete prog;
				return NULL;
			}
		}
		else
		{
			wsize = write(sock, "YUP", 3);

			#if DEBUG
			cout << "I got this prog: " << prog << endl;
			#endif
		}

		// Get kernels from prog
		vector<cl::Kernel> kerns;
		prog->createKernels(&kerns);

		char argname[30], ctype[30];

		// Set args
		int i;
		header >> arg >> arg >> arg >> ws;
		string key, val;

		// Parameter read stage.
		while (getline(header, key, '='))
		{
			char peeker = header.peek();

			// If we start with @, we're a buffer in our filesystem
			if (peeker == '@')
			{
				long buffSize = 0;
				char sizePeek, suffixPeek;

				header.ignore();
				sizePeek = header.peek();
				// If our buffer name starts with "[4k]" then create this 
				// 4k file buffer if it doesn't exist.
				if (sizePeek == '[')
				{
					header.ignore();
					// Expect a number and optional k or m
					header >> buffSize;
					suffixPeek = header.peek();
					if (suffixPeek == 'k')
					{
						buffSize *= 1024;
						header.ignore();
					}
					else if (suffixPeek == 'm')
					{
						buffSize *= 1024 * 1024;
						header.ignore();
					}

					if (header.get() != ']')
						cerr << "Missing close bracket on buffer size!";
				}

				// Get buffer path
				//getline(header, val, peeker);
				header >> val;

				MawNode *node = spair->state->root.Find(val);
				GPUNode *gpuNode = dynamic_cast<GPUNode*>(node);

				if (gpuNode)
				{
					paramsBuffer[key] = gpuNode;
					paramsRaw[key + "_size"] = to_string(gpuNode->stat.st_size);
				}
				else if (buffSize)
				{
					MawNode *dir = spair->state->root.Find(val, true);
					DirNode *dirNode = dynamic_cast<DirNode*>(dir);
					if (dirNode)
					{
						gpuNode = new GPUNode(buffSize);
						dirNode->Add(val, gpuNode);
						paramsBuffer[key] = gpuNode;
					}
				}
				paramsRaw[key] = (string)"@" + val;
			}
			else
			{
				header >> val;
				paramsRaw[key] = val;
			}
			
			header >> ws;
		}

		auto start = chrono::high_resolution_clock::now();

		// Queue each Kernel.
		// TODO add CPU options and synch.
		for (k = 0; k < kerns.size(); ++k)
		{
			// TODO sort kernels by name.  Currently queued in define order.
			string kernName;
			cl::Kernel kern = kerns[k];
			kern.getInfo<string>(CL_KERNEL_FUNCTION_NAME, &kernName);
			kernNames.push_back(kernName);

			#if DEBUG
			cout << "queuing kernel: " << kernName << endl;
			#endif

			// NEW WAY, using k/v map
			// Start with arg1, because arg0 is mawsh state
			// TODO error check for arg0 being mawsh state...
			cl_uint paramc = 0;
			kern.getInfo<cl_uint>(CL_KERNEL_NUM_ARGS, &paramc);
			char cname[32], ctype[20];
			string sname, stype, val;

			for (int i = 0; i < paramc; ++i)
			{
				res = kern.getArgInfo(i, CL_KERNEL_ARG_TYPE_NAME, &ctype);
				res = kern.getArgInfo(i, CL_KERNEL_ARG_NAME, &cname);
				sname = cname;
				stype = ctype;
				val  = paramsRaw[sname];

				// Is val undefined?
				if (!val.length())
				{
					cerr << "ERR couldn't set param " << sname << endl;
					continue;
				}

				#if DEBUG
				cout << CYAN << "Arg [" << cname << "] is type [" << ctype << "] value [" << val << "]\n" << RESET;
				#endif

				// Are we a literal/number?
				if (stype.find("*") == string::npos)
				{
					// Handle type-specific issues.
					if (val == "")
						cerr << "ERR empty param" << endl;
					if (stype == "int")
						kern.setArg(i, stoi(val));
					else if (stype == "float")
						kern.setArg(i, stof(val));
					else if (stype == "double")
						kern.setArg(i, stod(val));
					else if (stype == "long")
						kern.setArg(i, stol(val));
				}
				else if (stype == "mawproc*" || val[0] == '@')
				{
					GPUNode *node = paramsBuffer[cname];

					if (node)
						kern.setArg(i, node->buffer);
					else
						cerr << "ERROR: Problem setting kernel param buffer: " << sname << endl;
				}
				else	// We're a string *buffer specified inline. Copy to tmp device buffer.
				{
					kern.setArg(i, cl::Buffer(cl::Context::getDefault(), 
						CL_MEM_COPY_HOST_PTR, arg.length() + 1, (void*)val.c_str()));
				}
			}
	
			// Get the kernel run characteristics from kernname.
			// Examples: main, main_10x10 cpu_1 kernname_128x$F_8x8
			stringstream kstr(kernName);
			string str, ktype;
			size_t dims[2][3] = {{1,1,1},{1,1,1}};
			int pos = 0;
			int local = 0;
			getline(kstr, ktype, '_');

			// Experimental - CPU task returning control to the CPU.
			if (ktype == "cpu")
			{
				// Enqueue task and then call output via system.
				#if DEBUG
				cout << "System call." << endl;
				#endif
				system("echo TODO");
				continue;
			}

			// We're a kernel with optional dimensions.
			for (char peek = kstr.peek(); !kstr.eof(); peek = kstr.peek())
			{
				int val;

				// Dimension straight digits?
				if (isdigit(peek))
					kstr >> val;

				// Dimension one-char variable?
				else if (peek == '$')
				{
					kstr.ignore();
					char var = kstr.get();
					try
					{
						val = stoi(paramsRaw[&var]);
					}
					catch (exception e)
					{
						cerr << RED << "ERROR: Missing/bad kernel dimension. Using 1." 
							<< RESET << endl;
						val = 1;  
					}
				}
				// Shifting from global to group dimensions?
				else if (peek == '_')
				{
					local = 1;
					pos = 0;
					kstr.ignore();
					continue;
				}
				else if (peek == 'x')
				{
					kstr.ignore();
					continue;
				}

				dims[local][pos++] = val;
			}

			//clEnqueueNDRangeKernel(procNode.queue(), kern(),
			//	3, NULL, dims[0], dims[1], 0, NULL, NULL);

			int g0 = dims[0][0], g1 = dims[0][1], g2 = dims[0][2];
			int l0 = dims[1][0], l1 = dims[1][1], l2 = dims[1][2];
			procNode.queue.enqueueNDRangeKernel(kern, cl::NullRange, 
				//cl::NDRange(dims[0][0], dims[0][1], dims[0][2]), 
				cl::NDRange(g0, g1), 
				cl::NullRange, //cl::NDRange(dims[1][0], dims[1][1], dims[1][2]), 
				NULL, &timeEvents[k]);
		}

		char procbuf[ProcNode::procsize];

		// Explicit flush of queue for buffer mapping.
		// This is documented to be implicit but in practice varies by vendor.
		GPUNode *p = NULL;
		void *mbuf = NULL;

		procNode.queue.finish();

		// This slop is the kind of line only a mother could ❤. Sorry not sorry.
		// Read results of stdout (if any) using mapped buffer.
		if ((p = paramsBuffer["p"])
			&& (mbuf = procNode.queue.enqueueMapBuffer(p->buffer, CL_TRUE, CL_MAP_READ, 0, ProcNode::procsize)))
		{
			// Note we store length as int in first 4 bytes.
			int bufflen = *((int*)mbuf);
			int wsize = write(sock, ((char*)mbuf + sizeof(int)), bufflen);
			if (!wsize)
				cout << YELLOW << "Socket disconnected: " << sock << RESET << endl;
			else if (wsize == -1)
				cerr << RED << "Socket failed: " << sock << RESET << endl;
			
			procNode.queue.enqueueUnmapMemObject(p->buffer, mbuf);
		}

		spair->state->proc.Remove(dirname);

		// Timing
		#if DEBUG
		for (int i = 0; i < k; ++i)
		{
			cl_ulong start	= timeEvents[i].getProfilingInfo<CL_PROFILING_COMMAND_START>(),
					 end	= timeEvents[i].getProfilingInfo<CL_PROFILING_COMMAND_END>();
			cout << "kernel\t" << kernNames[i] << '\t' << end-start << "ns\t(" << start % 10000000 << " to " << end % 10000000 << ")" << endl;
		}

		auto finish = chrono::high_resolution_clock::now();
		cout << "Total\t" << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << "ns" << endl;
		#endif

		#if DEBUG
		cout << CYAN << "Connection finished: " << getpid() << '.' << pthread_self() << RESET << endl;
		#endif
	}
	catch (cl::Error e)
	{
		string err = (string)RED + "ERROR: " + GPUNode::ExplainCLError(e.err()) + '\n' + e.what() + RESET;

		write(sock, err.c_str(), err.length());
		close(sock);

		cerr << err << endl;
	}

	close(sock);
	return 0;
}

void* prep_socket(void *p_state)
{
	int client_sock, c;
	maws_state *state = (maws_state*)p_state;

	struct sockaddr_un server, client;
//	struct sockaddr_in server, client;
//	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	int server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	pthread_t thread_id;
	int res = 0;

	memset(&server, 0, sizeof(server));
	server.sun_family = AF_UNIX;
	if (state->sock == "")
		state->sock = "/tmp/mawd.sock";
	strncpy(server.sun_path, state->sock.c_str(), sizeof(server.sun_path)-1);
	unlink(state->sock.c_str());

	res = bind(server_sock, (struct sockaddr *)&server, sizeof(server));
	if (res < 0)
	{
		cerr << RED << "Bind failed. Error:" << res << RESET << endl;
		return 0;
	}

	listen(server_sock, 3);

	cout << GREEN << "Listening at " << state->sock << RESET << endl;
	c = sizeof(struct sockaddr_un);

	while (client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t*)&c))
	{
		// Set timeout
		struct timeval timeout;	
		timeout.tv_sec = 4;
	    timeout.tv_usec = 0;
		if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			cerr << RED << "setsockopt failed" << RESET << endl;
		if (setsockopt (client_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			cerr << RED << "setsockopt failed" << RESET << endl;

		socket_pair *sp = new socket_pair(client_sock, (maws_state*)state);

		if (pthread_create(&thread_id, NULL, connection_handler, sp) < 0)
			cerr << RED << "Could not create handler thread" << RESET << endl;
		
		#if DEBUG
		cout << CYAN << "Connection accepted: " << getpid() << '.' << thread_id << RESET << endl;
		#endif
	}

	if (client_sock < 0)
		cerr << RED << "Accept failed" << RESET << endl;
	
	return 0;
}
