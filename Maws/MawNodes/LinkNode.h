#ifndef LINKNODE
#define LINKNODE

#include <string>
#include <map>
#include <sys/stat.h>

#include "MawNode.h"

class LinkNode: public MawNode
{
  private:
	std::string fname;
  public:
	LinkNode();
	LinkNode(const char* path);
	virtual ~LinkNode();

	virtual int Read(char *buf, size_t size, off_t offset = 0) override;

	virtual int Write(const char *buf, size_t size, off_t offset = 0) override;

  public:

	virtual int Truncate(int newlen) override;
};

#endif
