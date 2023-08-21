# Welcome to the world of Mawenzy.

This project comprises the Mawenzy Daemon (Mawd) as well as the Mawenzy Shell client (Mawsh).  Together they serve as a development platform to help you take full advantage of your GPU using OpenCL.

To run the Mawd service, edit the env vars inside the "run.sh" script of ./Maws/bin/Release/run.sh and run it.  Set MAWS_PLAT to the desired index within clinfo listing.  Set MAWS_DEV to device index within that platform.  Defaults are 0,0.
To stop the Mawd service, fusermount -u /mnt/gpu or wherever you choose to mount.

There are systemd and upstart init files available if you want to use MawD at boot.

Example Mawsh scripts are included in the Mawsh project.

## Prereqs
For build you will need the following packages: fuse-devel, opencl-headers

For run you will need libFuse and libOpenCL with a valid device.
