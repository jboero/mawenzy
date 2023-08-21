#include "MawNode.h"
#include "ProcNode.h"

using namespace std;

ProcNode::ProcNode(int tid) : 
	DirNode(), 
	tid(tid), 
	queue(cl::Context::getDefault(), cl::Device::getDefault(), CL_QUEUE_PROFILING_ENABLE)
{
	DirNode::Add("buffer", &buf);

	buf.WriteExtend(procsize);
	buf.stat.st_size = procsize;

	// Note this requires OpenCL 1.2:
	queue.enqueueFillBuffer<char>(buf.buffer, 0, 0, procsize);
}

ProcNode::~ProcNode()
{
}
