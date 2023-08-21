/* 
 * This file is part of the Mawenzy Server (https://github.com/jboero/mawenzy).
 * Copyright (c) 2015 John Boero.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define FUSE_USE_VERSION 26

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>

#include <sys/socket.h>

#include <unistd.h>
#include <sys/xattr.h>

#include "Mawd.h"
#include "MawNodes/MawNode.h"
#include "MawNodes/DirNode.h"
#include "MawNodes/GPUNode.h"
#include "MawNodes/LinkNode.h"

#include "StdColors.h"
#include "Sockets.h"

#include <fuse.h>

#define MAWS_STATE ((struct maws_state *) fuse_get_context()->private_data)

using namespace std;

static const char* fullpath(const char *path)
{
	//cout << "STUB fullpath(" << path << ")=" << MAWS_STATE->backdir + path << endl;
	return (MAWS_STATE->backdir + path).c_str();
}

static int maws_error()
{
	cerr << RED << "ERROR " << " " << strerror(errno) << RESET << endl;
	return -errno;
}

int maws_getattr(const char *path, struct stat *statbuf)
{
	cout << CYAN << "maws_getattr path="<< path << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;
	else
		memcpy(statbuf, &(node->stat), sizeof(struct stat));
	return 0;
}

int maws_getxattr(const char *path, const char *name, char *value, size_t size)
{
	cout << CYAN << "maws_getxattr path="<< path << ": " << name << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;

	string xval = node->xattrs[name];
	xval.copy(value, xval.length(), 0);
	return xval.length();
}

int maws_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	cout << CYAN << "maws_setxattr path="<< path << ": " << name << "=" << value << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -1;
	
	node->xattrs[name] = value;
	return 0;
}

int maws_removexattr(const char *path, const char *name)
{
	cout << CYAN << "maws_removexattr path="<< path << ": " << name << endl;

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -1;

	node->xattrs.erase(name);
	return 0;
}

int maws_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_fgetattr fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	if (!strcmp(path, "/"))
		return fstat(fi->fh, statbuf) ? maws_error() : 0;

	return maws_getattr(path, statbuf);
}

int maws_listxattr(const char *path, char *list, size_t size)
{
	cout << CYAN << "maws_listxattr path="<< path << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);
	string xattrs;
	int curpos = 0;

	if (!node)
		return -1;
	
	for (map<string, string>::iterator it = node->xattrs.begin(); it != node->xattrs.end(); ++it)
	{
		string attr = it->first;
		attr.copy(list, attr.length(), curpos += attr.length());
		list[curpos++] = '\0';
	}

	return curpos;
}

/* TODO */
int maws_readlink(const char *path, char *link, size_t size)
{
	cout << RED << "maws_readlink (TODO) path=" << path << " link=" << link << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);
	LinkNode *lnode = (LinkNode*)node;

	if (!lnode)
		return -EINVAL;

	return lnode->Read(link, size);
}

/* TODO */
int maws_mknod(const char *path, mode_t mode, dev_t dev)
{
	cout << RED << "maws_mknod (TODO) path="<< path << " mode=" << mode << RESET << endl;
	int retstat = -EINVAL;
	const char *fpath = fullpath(path);

	if (S_ISREG(mode))
	{
		if (retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode))
			return maws_error();
		else if (close(retstat))
			return maws_error();
	}
	else if (S_ISFIFO(mode))
	{
		if (mkfifo(fpath, mode))
			return maws_error();
		else if (mknod(fpath, mode, dev))
			return maws_error();
	}
	return retstat;
}

int maws_mkdir(const char *path, mode_t mode)
{
	cout << CYAN << "maws_mkdir path="<< path << RESET << endl;

	string dirname(path), filename;
	DirNode *dir = (DirNode*)MAWS_STATE->root.Find(dirname, true);
	stringstream sstr(dirname);

	if (!dir)
		return -ENOENT;

	while (getline(sstr, filename, '/'));

	// Note mkdir does not explicitly include S_IFDIR
	dir->Add(filename, new DirNode(mode));

	return 0;
}

int maws_unlink(const char *path)
{
	cout << CYAN << "maws_unlink (removing) path="<< path << RESET << endl;
	string spath(path), filename;
	struct maws_state *state = MAWS_STATE;
	DirNode *dir = dynamic_cast<DirNode*>(state->root.Find(spath, true));
	MawNode *node;
	stringstream fname(spath);

	if (!dir)
		return -ENOENT;

	//dir->DebugPrint();
	while (getline(fname, filename, '/'));

	// Does it exist?
	if (!(node = dir->Find(filename)))
		return -ENOENT;
	// Is it a dir?
	if (dynamic_cast<DirNode*>(node))
		return -EISDIR;
	
	dir->Remove(filename);
	cout << YELLOW << "DELETING " << filename << " via " << path << RESET << endl;
	delete node;

	return 0;
}

int maws_rmdir(const char *path)
{
	cout << CYAN << "maws_rmdir path="<< path << RESET << endl;

	string dirname;
	struct maws_state *state = MAWS_STATE;
	MawNode *node = state->root.Find(path);
	DirNode *parent, *dirNode;
	stringstream ss(path);

	while (getline(ss, dirname, '/'));

	// Does it exist?
	if (!node)
		return -ENOENT;
	
	// Is the node a DirNode*?
	if (!(dirNode = dynamic_cast<DirNode*>(node)))
		return -ENOTDIR;

	// Is parent a dir?
	MawNode *pnode = dirNode->subnodes[".."];
	if (!(parent = dynamic_cast<DirNode*>(pnode)))
		return -EINVAL;

	// TODO sort this out.
	// Is the dir empty?
	if (dirNode->GetSize())
		return -ENOTEMPTY;
	
	parent->Remove(dirname);
	delete dirNode;

	return 0;
}

// Note this is currently a faux hardlink - symlink TODO
int maws_symlink(const char *path, const char *link)
{
	cout << CYAN << "maws_symlink (HARDLINKING) path=" << path << " to " << link << RESET << endl;

	stringstream newss(link);
	string linkname;
	DirNode *olddir = (DirNode*)MAWS_STATE->root.Find(path, true);
	DirNode *linkdir = (DirNode*)MAWS_STATE->root.Find(link, true);
	MawNode *node = MAWS_STATE->root.Find(path);
	MawNode *dest = MAWS_STATE->root.Find(link);

	while (getline(newss, linkname, '/'));

	// Oldpath and newpath must exist
	if (!olddir || !linkdir || !node)
		return -ENOENT;
	if (dest)
		return -EEXIST;

	linkdir->Add(linkname, new LinkNode(path));
	return 0;
}

int maws_rename(const char *path, const char *newpath)
{
	cout << CYAN << "maws_rename " << path << " to " << newpath << RESET << endl;

	stringstream oldss(path), newss(newpath);
	string oldname, newname;
	DirNode *olddir = (DirNode*)MAWS_STATE->root.Find(path, true);
	DirNode *newdir = (DirNode*)MAWS_STATE->root.Find(newpath, true);
	MawNode *node = MAWS_STATE->root.Find(path);

	while (getline(oldss, oldname, '/'));
	while (getline(newss, newname, '/'));

	// Oldpath and newpath must exist
	if (!olddir || !newdir)
		return -EINVAL;
	
	// Don't rename the root node! Consuela no-no-no
	if (!node || node == &(MAWS_STATE->root))
		return -EINVAL;
	
	olddir->Remove(oldname);
	newdir->Add(newname, node);
	return 0;
}

int maws_link(const char *path, const char *newpath)
{
	cout << RED << "maws_link (TODO) path="<< path << RESET << endl;
	return link(fullpath(path), fullpath(newpath)) ? maws_error() : 0;
}

int maws_chmod(const char *path, mode_t mode)
{
	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;

	node->stat.st_mode = mode;

	return 0;
}

int maws_chown(const char *path, uid_t uid, gid_t gid)  
{
	cout << RED << "maws_chown path="<< path << RESET << endl;
	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;

	node->stat.st_uid = uid;
	node->stat.st_gid = gid;

	return 0;
}

int maws_truncate(const char *path, off_t newsize)
{
	#if DEBUG
	cout << CYAN << "maws_truncate path="<< path << " newsize=" << newsize << RESET << endl;
	#endif

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;
	
	return node->Truncate(newsize);
}

// TODO
int maws_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_ftruncate (TODO) fi->fh[" << fi->fh << "] path="<< path << RESET << endl;
	return ftruncate(fi->fh, offset) < 0 ? maws_error() : 0;
}

int maws_utime(const char *path, struct utimbuf *ubuf)
{
	cout << CYAN << "maws_utime path="<< path << RESET << endl;

	string filename = string(path);
	struct maws_state *state = MAWS_STATE;
	cl::Buffer newBuff;
	MawNode *node = state->root.Find(filename);

	if (!node)
		return -ENOENT;

	node->stat.st_mtime = time(NULL);
	return 0;
}

int maws_open(const char *path, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_open fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	MawNode *node = MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;

	node->Open(fi->fh);

	return 0;
}

int maws_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	#if DEBUG
	cout << CYAN << "maws_read path="<< path << " " << offset << " - " << offset + size << endl;
	#endif

	MawNode *node = MawNode::open_nodes[fi->fh];

	// File does not appear to be open?
	if (!node)
		return -EINVAL;

	return node->Read(buf, size, offset);
}

int maws_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	#if DEBUG
	cout << CYAN << "maws_write fi->fh[" << fi->fh << "] path="<< path << RESET << endl;
	#endif

	struct maws_state *state = MAWS_STATE;
	string filename(path);
	MawNode *node = MawNode::open_nodes[fi->fh];

	// Is the file open?
	if (!node)
		return -EINVAL;
	
	return node->Write(buf, size, offset);
}

// Return stat of root fs.
int maws_statfs(const char *path, struct statvfs *statv)
{
	cout << CYAN << "maws_statfs path="<< path << RESET << endl;
	MawNode *node = MAWS_STATE->root.Find(path);

	if (node && statv)
	{
		/*
		unsigned long f_bsize    File system block size. 
		unsigned long f_frsize   Fundamental file system block size. 
		fsblkcnt_t    f_blocks   Total number of blocks on file system in units of f_frsize. 
		fsblkcnt_t    f_bfree    Total number of free blocks. 
		fsblkcnt_t    f_bavail   Number of free blocks available to 
		                         non-privileged process. 
		fsfilcnt_t    f_files    Total number of file serial numbers. 
		fsfilcnt_t    f_ffree    Total number of free file serial numbers. 
		fsfilcnt_t    f_favail   Number of file serial numbers available to 
		                         non-privileged process. 
		unsigned long f_fsid     File system ID. 
		unsigned long f_flag     Bit mask of f_flag values. 
		unsigned long f_namemax  Maximum filename length. 
		*/
		statv->f_bsize	= 4096;
		statv->f_frsize	= 4096;
		statv->f_blocks	= 4096;
		statv->f_bfree	= 4096;
		statv->f_bavail	= 4096;
		statv->f_files	= 15;
		statv->f_bfree	= 15;
		statv->f_favail	= 10000;
		statv->f_fsid	= 100;
		statv->f_flag	= 0;
		statv->f_namemax = 0xFFFF;
		return 0;
	}

	return -1;
}

int maws_flush(const char *path, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_flush fi->fh[" << fi->fh << "] path="<< path << RESET << endl;
	return 0;
}

int maws_release(const char *path, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_release fi->fh[" << fi->fh << "] path="<< path << RESET << endl;
	MawNode *node = MawNode::open_nodes[fi->fh];

	if (!node)
		return 0;

	return node->Release(fi->fh);
}

int maws_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_fsync fi->flh[" << fi->fh << "] path="<< path << RESET << endl;
	return 0;
}

int maws_opendir(const char *path, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_opendir fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	DirNode *node = (DirNode*)MAWS_STATE->root.Find(path);

	if (!node)
		return -ENOENT;
	node->Open(fi->fh);

	return 0;
}

int maws_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_readdir fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	DirNode *node = DirNode::GetOpenHandle(fi->fh);

	if (!node)
		return -ENOENT;
	
	return node->Read(buf, filler, offset, fi);
}

int maws_releasedir(const char *path, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_releasedir fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	DirNode::CloseHandle(fi->fh);
	return 0;
}

int maws_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_fsyncdir fi->fh[" << fi->fh << "] path="<< path << RESET << endl;
	return 0;
}

void printf_callback( const char *buffer, size_t len, size_t complete, void *user_data )
{
	// TODO thread isolation.
	printf( "%.*s", len, buffer );
}

void* maws_init(struct fuse_conn_info *conn)
{
	int platno = 0, devno = 0;
	maws_state *state = MAWS_STATE;
	char *envar;

	try
	{
		if (envar = getenv("MAWS_PLAT"))
			platno = atoi(envar);
		if (envar = getenv("MAWS_DEV"))
			devno = atoi(envar);
		if (envar = getenv("MAWS_BASE"))
			state->rootdir = envar;
		if (envar = getenv("MAWS_COMMON"))
			state->commonsrc = envar;
		if (envar = getenv("MAWS_SOCK"))
			state->sock = envar ? envar : "/tmp/mawd.sock";
		if (envar = getenv("MAWS_SRCDIR"))
			state->backdir = envar;
		
		vector<cl::Platform> all_platforms;
		vector<cl::Device> all_devices;

		cl::Platform::get(&all_platforms);
		for (int i = 0; i < all_platforms.size(); ++i)
		{
			cl::Platform plat = all_platforms[i];
			cout << CYAN << "PLATFORM[" << i << "]: " 
				<< plat.getInfo<CL_PLATFORM_NAME>() << "\t"
				<< plat.getInfo<CL_PLATFORM_VERSION>() << RESET << endl;

			plat.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
			for (int j = 0; j < all_devices.size(); ++j)
				cout << CYAN << "\tDEVICE[" << j << "]: " 
					<< all_devices[j].getInfo<CL_DEVICE_NAME>() << RESET << endl;
		}

		if (platno >= all_platforms.size())
		{
			cerr << YELLOW << "WARN: MAWS_PLAT out of range. Trying 0." << RESET << endl;
			platno = 0;
		}
		cl::Platform def_plat = all_platforms[platno];
		def_plat.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);

		if (devno >= all_devices.size())
		{
			cerr << YELLOW << "WARN: MAWS_DEV out of range. Trying 0." << RESET << endl;
			devno = 0;
		}
		cl::Device def_dev = all_devices[devno];

		cout << CYAN << "Selected platform: " << def_plat.getInfo<CL_PLATFORM_NAME>() << endl;
		cout << "Selected device: " << def_dev.getInfo<CL_DEVICE_NAME>() << RESET << endl << endl;

		cl_context_properties *properties = NULL;
		/* Optional context properties for experimentation.
		cl_context_properties properties[] =
		{
			CL_PRINTF_CALLBACK_ARM,   (cl_context_properties) printf_callback,
			CL_PRINTF_BUFFERSIZE_ARM, (cl_context_properties) 0x100000,
			CL_CONTEXT_PLATFORM,      (cl_context_properties) def_plat(),
			0
		};*/

		cl::Context def_context(def_dev, properties);
		cl::CommandQueue def_que(def_context, def_dev, CL_QUEUE_PROFILING_ENABLE);

		// TODO get queue count from env
		//for (int i = 0; i < maws_state::queue_count; ++i)
		//	state->queues[i] = cl::CommandQueue(def_context, def_dev);

		// Set defaults for simplicity.
		cl::Platform::setDefault(def_plat);
		cl::Context::setDefault(def_context);
		cl::Device::setDefault(def_dev);
		cl::CommandQueue::setDefault(def_que);

		state->root.Add("sys", &(state->sys));
		state->root.Add("proc", &(state->proc));
	}
	catch (cl::Error e)
	{
		cerr << RED << "ERR get_context. Trying to resume, but: " << endl << e.what() 
			<< " : " << e.err() << ": " << GPUNode::ExplainCLError(e.err()) << RESET << endl;
	}

	pthread_t listener;
	if (pthread_create(&listener, NULL, prep_socket, (void*)MAWS_STATE) < 0)
		cerr << RED << "ERROR Creating listener thread" << RESET << endl;
	
	// TODO check dir valid and prevent injection.
	if (state->backdir != "")
	{
		string cmd = string("rsync -a --exclude=/proc --exclude=/sys '") + state->backdir + "' '" + state->rootdir + "' &";
		cout << cmd << endl;
		if (system(cmd.c_str()))
			cerr << "WARNING: Unable to sync backdir '" << state->backdir << "'\n";
	}

	return state;
}

void maws_destroy(void *userdata)
{
	// RSYNC contents back to disk.
	// TODO find timing for this.  INodes have been destroyed at this point.
	if (MAWS_STATE->backdir != "")
	{
		string cmd = string("rsync -a --exclude=/sys --exclude=/proc '" + MAWS_STATE->rootdir + "' '") + MAWS_STATE->backdir + "'";
		cout << cmd << endl;
		if (system(cmd.c_str()))
			cerr << "WARNING unable to rsync contents to '" << MAWS_STATE->backdir << endl;
	}

	// Context handled by destructors.
	if (MAWS_STATE)
		delete MAWS_STATE;
}

int maws_access(const char *path, int mask)
{
	cout << CYAN << "maws_access IGNORED path="<< path << " mask " << hex << mask << RESET << endl;
	return 0;
}

int maws_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	cout << CYAN << "maws_create fi->fh[" << fi->fh << "] path="<< path << RESET << endl;

	string filename(path);
	DirNode *dir = (DirNode*)MAWS_STATE->root.Find(path, true);
	MawNode *node;
	stringstream ss(filename);

	if (!dir)
		return -EINVAL;
	
	while (getline(ss, filename, '/'));

	node = new GPUNode();
	node->stat.st_mode |= mode;

	dir->Add(filename, node);

	// Note assumed created open...
	MawNode::open_nodes[fi->fh] = node;
	return 0;
}

int maws_lock(const char *path, fuse_file_info* finfo, int whatever, flock* locker)
{
	// Ignore locking for now.
	cout << YELLOW << "LOCK " << path << " " << locker->l_type << RESET << endl;
	return 0;
}

void maws_usage()
{
	cout << "USAGE, dawg" << endl;
	abort();
}

int main(int argc, char *argv[])
{
	maws_state *state = new maws_state();
	struct fuse_operations fuse = 
	{
		.getattr = maws_getattr,
		.readlink = maws_readlink,
		.getdir = NULL,
		.mknod = maws_mknod,
		.mkdir = maws_mkdir,
		.unlink = maws_unlink,
		.rmdir = maws_rmdir,
		.symlink = maws_symlink,
		.rename = maws_rename,
		.link = maws_link,
		.chmod = maws_chmod,
		.chown = maws_chown,
		.truncate = maws_truncate,
		.utime = maws_utime,
		.open = maws_open,
		.read = maws_read,
		.write = maws_write,
		.statfs = maws_statfs,
//		.flush = maws_flush,
		.flush = NULL,
		.release = maws_release,
//		.release = NULL,

		.fsync = NULL,

/**/	.setxattr = maws_setxattr,
		.getxattr = maws_getxattr,
		.listxattr = maws_listxattr,
		.removexattr = maws_removexattr,
/** /	.setxattr = NULL,
		.getxattr = NULL,
		.listxattr = NULL,
		.removexattr = NULL,
/**/

		.opendir = maws_opendir,
		.readdir = maws_readdir,
		.releasedir = maws_releasedir,
		.fsyncdir = NULL,
		.init = maws_init,
		.destroy = maws_destroy,
		.access = maws_access,
//		.access = NULL,
		.create = maws_create,
		.ftruncate = maws_ftruncate,
		.fgetattr = maws_fgetattr,
		.lock = maws_lock
	};

	if (argc < 3)
		maws_usage();

	if ((getuid() == 0) || (geteuid() == 0))
		cerr << YELLOW << "WARNING Running a FUSE filesystem as root opens security holes" << endl;
	
	return fuse_main(argc, argv, &fuse, state);
}
