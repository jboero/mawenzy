#include <unistd.h>
#include "../StdColors.h"
#include "MawNode.h"

// Static init
std::map<int, MawNode*> MawNode::open_nodes = std::map<int, MawNode*>();

MawNode::MawNode()
{
	stat.st_uid = getuid();
	stat.st_gid = getgid();
	stat.st_nlink = 1;
	stat.st_size = stat.st_blocks = 0;
	stat.st_atime = stat.st_mtime = stat.st_ctime = time(NULL);
	stat.st_blksize = MAWSBLKSIZE;
	stat.st_mode = S_IFREG | 0660;
}

MawNode::~MawNode()
{
}

const struct stat MawNode::GetStat()
{
	return stat;
}

void MawNode::Open(int fh)
{
	open_nodes[fh] = this;
}

int MawNode::Release(int fh)
{
	open_nodes.erase(fh);
	return 0;
}

int MawNode::Read(char *buf, size_t size, off_t offset)
{
	return -EINVAL;
}

int MawNode::Write(const char *buf, size_t size, off_t offset)
{
	stat.st_mtime = time(NULL);
	return size;
}

int MawNode::Truncate(int newsize)
{
	return 0;
}
