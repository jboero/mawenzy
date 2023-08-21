#ifndef GPUNODE
#define GPUNODE

#include "MawNode.h"

#include <string>
#include <map>
#include <sys/stat.h>
#include <CL/cl2.hpp>

class GPUNode: public MawNode
{
  public:		// TODO: protect this.
	cl::Buffer buffer;

  public:
	GPUNode();
	GPUNode(int size);
	virtual ~GPUNode();

	virtual int Read(char *buf, size_t size, off_t offset, const cl::CommandQueue &queue);
	virtual int Read(char *buf, size_t size, off_t offset = 0) override;

	virtual int Write(const char *buf, size_t size, off_t offset, const cl::CommandQueue &queue);
	virtual int Write(const char *buf, size_t size, off_t offset = 0) override;

	static std::string ExplainCLError(cl_int err);
  public:
  	void WriteExtend(int newlen);

	virtual int Truncate(int newlen) override;
};

#endif
