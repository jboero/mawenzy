#ifndef MAWNODE
#define MAWNODE

#define CL_MINIMUM_OPENCL_VERSION		120
#define CL_TARGET_OPENCL_VERSION		200
#define CL_HPP_TARGET_OPENCL_VERSION	200
#define CL_HPP_ENABLE_EXCEPTIONS

#include <string>
#include <map>
#include <sys/stat.h>

#define MAWSBLKSIZE 4096

class MawNode
{
  public:
	struct stat stat;
	std::map<std::string, std::string> xattrs;
	MawNode *parent;

  public:
	static std::map<int, MawNode*> open_nodes;

	MawNode();
	virtual ~MawNode();

	virtual const struct stat GetStat();

	virtual void Open(int fh);

	virtual int Release(int fh);

	virtual int Read(char *buf, size_t size, off_t offset);

	virtual int Write(const char *buf, size_t size, off_t offset);

	virtual int Truncate(int newlen);
};

#endif
