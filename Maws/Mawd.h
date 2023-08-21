#ifndef OLDMAWNODE
#define OLDMAWNODE

//#include "cl_ext.h"

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <sys/stat.h>

#include "MawNodes/MawNode.h"
#include "MawNodes/DirNode.h"
#include "MawNodes/ProcNode.h"
#include "MawNodes/SysNode.h"
#include <CL/cl2.hpp>

struct maws_state
{
	// Shared queue pool quantity (optional)
	static const int queue_count = 20;

	std::string rootdir;
	std::string backdir;
	std::string commonsrc = "common_src.cl";
	std::string sock;
	DirNode root, proc;
	SysNode sys;

	cl::CommandQueue queues[queue_count];
	cl::Program::Sources common_src;
	cl::Program common;

	// Map of all loaded programs by inode|mtime
	std::map<std::string, cl::Program*> knowns;
};

#define MAWS_STATE ((struct maws_state *) fuse_get_context()->private_data)
#endif
