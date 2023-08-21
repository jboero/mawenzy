#include <fuse.h>
#include <queue>

#include "MawNode.h"
#include <sstream>

#ifndef DIRNODE
#define DIRNODE

class DirNode : public MawNode
{
  public:
	// Inherits the following
	// struct stat stat;
	// map<std::string, std::string> xattrs;

	std::map<std::string, MawNode*> subnodes;

  private:
	static std::map<int, DirNode*> open_nodes;

	virtual MawNode* Find(std::stringstream &pather, bool finddir);

  public:
	DirNode();
	DirNode(mode_t mode);

	virtual ~DirNode();

	virtual void Open(int fh);

	static DirNode* GetOpenHandle(int fh);

	static int CloseHandle(int fh);

	virtual int Read(void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

	// Return null if not found.
	virtual MawNode* Find(const std::string &path, bool finddir = false);

	// Add a subnode.
	void Add(const std::string &name, MawNode *node);

	// Remove a subnode (don't delete it from memory though).
	void Remove(const std::string &name);

	// Gets the quantity of our dir contents.
	int GetSize();

	void DebugPrint();
};

#endif
