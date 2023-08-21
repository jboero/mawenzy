#include "LinkNode.h"
#include "../StdColors.h"

using namespace std;

LinkNode::LinkNode() : MawNode()
{
	stat.st_mode |= S_IFLNK;
}

LinkNode::LinkNode(const char* path) : LinkNode()
{
	fname = path;
}

LinkNode::~LinkNode()
{
}

// Read from our cl buffer using default command queue.
int LinkNode::Read(char *buf, size_t size, off_t offset)
{
	int actual = min(fname.length(), size);

	fname.copy(buf, actual);
	return actual;
}

// Write to our cl buffer.
int LinkNode::Write(const char *buf, size_t size, off_t offset)
{
	fname = buf;
	return fname.length();
}

int LinkNode::Truncate(int newlen)
{
	fname = "";
	return 0;
}
