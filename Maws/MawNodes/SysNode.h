#ifndef SYSNODE
#define SYSNODE

#include <string>
#include <map>
#include <sys/stat.h>

#include "GPUNode.h"
#include "DirNode.h"

class SysNode: public DirNode
{
  public:
	SysNode();
	virtual ~SysNode();

	virtual int Read(void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);


};

#endif
