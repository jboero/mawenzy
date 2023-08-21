#include <iostream>
#include <sstream>
#include "../StdColors.h"
#include "MawNode.h"
#include "DirNode.h"

using namespace std;

std::map<int, DirNode*> DirNode::open_nodes = std::map<int, DirNode*>();

DirNode::DirNode() : MawNode()
{
	stat.st_nlink = 2;
	stat.st_size = 4096;

	// Make sure not IFREG, but IFDIR
	stat.st_mode = S_IFDIR | 0755;

	subnodes["."] = this;
}

DirNode::DirNode(mode_t mode) : MawNode()
{
	stat.st_nlink = 2;
	stat.st_size = 4096;
	stat.st_mode = S_IFDIR | mode;

	subnodes["."] = this;
}

DirNode::~DirNode()
{
}

void DirNode::Open(int fh)
{
	open_nodes[fh] = this;
}

DirNode* DirNode::GetOpenHandle(int fh)
{
	return open_nodes[fh];
}

int DirNode::CloseHandle(int fh)
{
	open_nodes.erase(fh);
	return 0;
}

// Note this is a differnet read() for directories.  base read still returns -EINVAL
int DirNode::Read(void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	for (map<string, MawNode*>::iterator it = subnodes.begin(); it != subnodes.end(); it++)
		filler(buf, it->first.c_str(), &it->second->stat, 0);
	
	return 0;
}

// Return null if not found.
MawNode* DirNode::Find(const string &path, bool finddir)
{
	if (path.length() < 1)
		return NULL;

	stringstream ss(path);

	return Find(ss, finddir);
}

MawNode* DirNode::Find(stringstream &spath, bool finddir)
{
	string dirname;

	// Note "." and ".." should be valid subnodes.
	if (spath.eof())
		return finddir ? subnodes[".."] : this;
	
	getline(spath, dirname, '/');

	// Is dir a blank (double slash?)
	if (!dirname.length())
		return Find(spath, finddir);

	// fsck autos...
	map<string,MawNode*>::iterator subber = subnodes.find(dirname);

	// Is this the end of path?
	if (spath.eof())
	{
		if (finddir)
			return this;
		
		// Find subnodes without [] which will create them.
    	if(subber != subnodes.end())
    		return subber->second;
	}
	else if (subber != subnodes.end())
	{
		MawNode *subnode = subber->second;
		DirNode *subdir = dynamic_cast<DirNode*>(subnode);
		if (subdir)
			return subdir->Find(spath, finddir);
	}
	return NULL;
}

// Add a subnode.
void DirNode::Add(const string &name, MawNode *node)
{
	DirNode *dirnode = dynamic_cast<DirNode*>(node);

	subnodes[name] = node;

	// DirNode redundant parent..
	if (dirnode)
		dirnode->subnodes[".."] = this;

	node->parent = this;
}

// Remove a subnode (don't delete it from memory though).
void DirNode::Remove(const std::string &name)
{
	MawNode *node = Find(name);

	if (node)
	{
		DirNode *dirnode;
		node->parent = NULL;
		// TODO wtf was I thinking?
		//if (dirnode = dynamic_cast<DirNode*>(dirnode))
		//	dirnode->subnodes[".."] = NULL;
	}
	subnodes.erase(name);
}

// Get our subnode quantity.
int DirNode::GetSize()
{
	return subnodes.size() - 2;
}

void DirNode::DebugPrint()
{
	cout << YELLOW << "DirNode::DebugPrint: " << hex;
	for (DirNode *node = this; node; node = dynamic_cast<DirNode*>(node->subnodes[".."]))
		cout << node << "<-";

	cout << endl << "Subnodes: " << endl;
	for (map<string, MawNode*>::iterator it = subnodes.begin(); it != subnodes.end(); ++it)
		cout << it->first << "(" << it->second << ")" << endl;
	cout << dec << RESET << endl;
}