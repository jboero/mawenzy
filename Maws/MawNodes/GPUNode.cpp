#include <iostream>
#include "GPUNode.h"
#include "../StdColors.h"

using namespace std;

GPUNode::GPUNode() : MawNode()
{
	stat.st_mode |= S_IFREG;
	buffer = cl::Buffer(cl::Context::getDefault(), CL_MEM_READ_WRITE, 4096);
}

GPUNode::GPUNode(int size) : GPUNode()
{
	WriteExtend(size);

	// Note some implmentations zero new buffers.
	cl::CommandQueue::getDefault().enqueueFillBuffer<char>(buffer, 0, 0, size);
}

GPUNode::~GPUNode()
{
	// implicit buffer destructor
}

// Read from our cl buffer using a specific command queue.  Useful for ProcNode synchronization.
int GPUNode::Read(char *buf, size_t size, off_t offset, const cl::CommandQueue &queue)
{
	int maxread = -EOF;

	if (offset >= stat.st_size)
		return -EOF;
	try
	{
		maxread = std::min(size, stat.st_size - (size_t)offset);

		void *mbuf = queue.enqueueMapBuffer(buffer, CL_FALSE, CL_MAP_READ, offset, size);
		if (mbuf)
			memcpy(buf, mbuf, size);
		queue.enqueueUnmapMemObject(buffer, mbuf);
 	}
	catch (cl::Error e)
	{
		cerr << RED << "ERR maws_read: " << e.what() << " : " << e.err() << RESET << endl;
		return -EINVAL;
	}
	return maxread;
}

// Read from our cl buffer using default command queue.
int GPUNode::Read(char *buf, size_t size, off_t offset)
{
	return Read(buf, size, offset, cl::CommandQueue::getDefault());
}

int GPUNode::Write(const char *buf, size_t size, off_t offset, const cl::CommandQueue &queue)
{
	try
	{
		if (stat.st_blocks * stat.st_blksize < offset + size)
			WriteExtend(offset + size);
		
		void *mbuf = queue.enqueueMapBuffer(buffer, CL_FALSE, CL_MAP_WRITE, offset, size);
		if (mbuf)
			memcpy(mbuf, buf, size);
		queue.enqueueUnmapMemObject(buffer, mbuf);

		stat.st_size = max((int)stat.st_size, (int)(offset + size));
	}
	catch (cl::Error e)
	{
		cerr << RED << "ERR write: " << e.what() << " : " << ExplainCLError(e.err()) << RESET << endl;
		return -EINVAL;
	}

	return size;
}

// Write to our cl buffer.
int GPUNode::Write(const char *buf, size_t size, off_t offset)
{
	return Write(buf, size, offset, cl::CommandQueue::getDefault());
}

// Resize buffer exponentially based on block usage.
// Throws potential cl::Error
void GPUNode::WriteExtend(int newlen)
{
	// Use quadratic growth for starters (ala std::vector).
	int newblks;
	for (newblks = 1; newblks * stat.st_blksize < newlen; newblks *= 2);

	if (newblks > stat.st_blocks)
	{
		cl::Buffer newBuff(cl::Context::getDefault(), CL_MEM_READ_WRITE, newblks * stat.st_blksize);

		if (stat.st_size > 0)
			cl::CommandQueue::getDefault().enqueueCopyBuffer(buffer, newBuff, 0, 0, stat.st_size);

		// TODO: ensure buffer purge
		buffer = newBuff;
		stat.st_blocks = newblks;
		cout << YELLOW << std::dec << "write_extend buffer size: " << newblks * stat.st_blksize << RESET << endl;
	}
	stat.st_size = newlen;
}

int GPUNode::Truncate(int newlen)
{
	// Use quadratic growth for starters (ala std::vector).
	int newblks;
	for (newblks = 1; newblks * stat.st_blksize < newlen; newblks *= 2);

	try
	{
		cl::Buffer newBuff(cl::Context::getDefault(), CL_MEM_READ_WRITE, newblks * stat.st_blksize);

		if (newlen > 0)
			cl::CommandQueue::getDefault().enqueueCopyBuffer(buffer, newBuff, 0, 0, std::min(newlen, (int)stat.st_size));

		// TODO: ensure buffer purge
		buffer = newBuff;
		stat.st_blocks = newblks;
		stat.st_size = newlen;
		cout << YELLOW << std::dec << "truncate: newblocks=" << newblks << RESET << endl;
	}
	catch (cl::Error e)
	{
		cerr << RED << "ERR write: " << e.what() << " : " << e.err() << RESET << endl;
		return -EINVAL;
	}

	return newlen;
}

std::string GPUNode::ExplainCLError(cl_int err)
{
	switch (err)
	{
		case 0:
			return "CL_SUCCESS";
		case -1:
			return "CL_DEVICE_NOT_FOUND";
		case -2:
			return "CL_DEVICE_NOT_AVAILABLE";
		case -3:
			return "CL_COMPILER_NOT_AVAILABLE";
		case -4:
			return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		case -5:
			return "CL_OUT_OF_RESOURCES";
		case -6:
			return "CL_OUT_OF_HOST_MEMORY";
		case -7:
			return "CL_PROFILING_INFO_NOT_AVAILABLE";
		case -8:
			return "CL_MEM_COPY_OVERLAP";
		case -9:
			return "CL_IMAGE_FORMAT_MISMATCH";

		case -10:
			return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		case -11:
			return "CL_BUILD_PROGRAM_FAILURE";
		case -12:
			return "CL_MAP_FAILURE";
		case -13:
			return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		case -14:
			return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		case -15:
			return "CL_COMPILE_PROGRAM_FAILURE";
		case -16:
			return "CL_LINKER_NOT_AVAILABLE";
		case -17:
			return "CL_LINK_PROGRAM_FAILURE";
		case -18:
			return "CL_DEVICE_PARTITION_FAILED";
		case -19:
			return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		case -30:
			return "CL_INVALID_VALUE";
		case -31:
			return "CL_INVALID_DEVICE_TYPE";
		case -32:
			return "CL_INVALID_PLATFORM";
		case -33:
			return "CL_INVALID_DEVICE";
		case -34:
			return "CL_INVALID_CONTEXT";
		case -35:
			return "CL_INVALID_QUEUE_PROPERTIES";
		case -36:
			return "CL_INVALID_COMMAND_QUEUE";
		case -37:
			return "CL_INVALID_HOST_PTR";
		case -38:
			return "CL_INVALID_MEM_OBJECT";
		case -39:
			return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";

		case -40:
			return "CL_INVALID_IMAGE_SIZE";
		case -41:
			return "CL_INVALID_SAMPLER";
		case -42:
			return "CL_INVALID_BINARY";
		case -43:
			return "CL_INVALID_BUILD_OPTIONS";
		case -44:
			return "CL_INVALID_PROGRAM";
		case -45:
			return "CL_INVALID_PROGRAM_EXECUTABLE";
		case -46:
			return "CL_INVALID_KERNEL_NAME";
		case -47:
			return "CL_INVALID_KERNEL_DEFINITION";
		case -48:
			return "CL_INVALID_KERNEL";
		case -49:
			return "CL_INVALID_ARG_INDEX";

		case -50:
			return "CL_INVALID_ARG_VALUE";
		case -51:
			return "CL_INVALID_ARG_SIZE";
		case -52:
			return "CL_INVALID_KERNEL_ARGS";
		case -53:
			return "CL_INVALID_WORK_DIMENSION";
		case -54:
			return "CL_INVALID_WORK_GROUP_SIZE";
		case -55:
			return "CL_INVALID_WORK_ITEM_SIZE";
		case -56:
			return "CL_INVALID_GLOBAL_OFFSET";
		case -57:
			return "CL_INVALID_EVENT_WAIT_LIST";
		case -58:
			return "CL_INVALID_EVENT";
		case -59:
			return "CL_INVALID_OPERATION";

		case -60:
			return "CL_INVALID_GL_OBJECT";
		case -61:
			return "CL_INVALID_BUFFER_SIZE";
		case -62:
			return "CL_INVALID_MIP_LEVEL";
		case -63:
			return "CL_INVALID_GLOBAL_WORK_SIZE";
		case -64:
			return "CL_INVALID_PROPERTY";
		case -65:
			return "CL_INVALID_IMAGE_DESCRIPTOR";
		case -66:
			return "CL_INVALID_COMPILER_OPTIONS";
		case -67:
			return "CL_INVALID_LINKER_OPTIONS";
		case -68:
			return "CL_INVALID_DEVICE_PARTITION_COUNT";
		case -69:
			return "CL_INVALID_PIPE_SIZE";
		case -70:
			return "CL_INVALID_DEVICE_QUEUE";

		default:
			return "UNKOWN";
	}
}
