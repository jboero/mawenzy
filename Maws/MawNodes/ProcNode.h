#ifndef PROCNODE
#define PROCNODE

#include "MawNode.h"
#include "GPUNode.h"
#include "DirNode.h"
#include <fuse.h>

// Simple DirNode with aggregate GPUNodes for stdout, stderr, TODO: env/params, subbuffernode.
class ProcNode: public DirNode
{
  public:
	// TODO protect these a bit?
	// 16k Proc buff for now.
	static const int procsize = 0x10000;
	int tid;
	GPUNode	buf;
	cl::CommandQueue queue;

  public:
	ProcNode(int tid);
	virtual ~ProcNode();
};

#endif
