Name:           mawenzy
Version:        0.3
Release:        1%{?dist}
Summary:        Mawenzy app server and client for GPU.

License: GPLv3
Source0: mawenzy.tar.gz
URL: https://www.github.com/jboero/mawenzy
BuildRequires: make gcc-c++ fuse-devel opencl-filesystem opencl-headers ocl-icd-devel
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
        Mawenzy server and client for OpenCL applications. Examples included.

%global debug_package %{nil}

%prep
        %autosetup -c %{name}-%{version}

%build
        make -j2

%pre
        getent group %{name} > /dev/null || groupadd -r %{name}
        getent passwd %{name} > /dev/null || \
        useradd -r -d %{_sharedstatedir}/%{name} -g %{name} \
                -s /sbin/nologin -c "Mawenzy service user." %{name}
        exit 0

%install
        mkdir -p %{buildroot}%{_bindir}
        cp -p Maws/bin/Debug/mawd %{buildroot}%{_bindir}
        cp -p Mawsh/bin/Debug/mawsh     %{buildroot}%{_bindir}

        mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
        cp -p Maws/Config/mawd.conf %{buildroot}%{_sysconfdir}/sysconfig

        mkdir -p %{buildroot}%{_includedir}/mawsh
        cp Maws/MawshCL/*.cl %{buildroot}%{_includedir}/mawsh

		# _unitdir isn't set in some environments.... frustrating
        mkdir -p %{buildroot}/usr/lib/systemd/system
        cp -p Maws/Config/mawd.service %{buildroot}/usr/lib/systemd/system

        mkdir -p %{buildroot}/opt/mawsh/examples
        cp -p Examples/*.mawsh %{buildroot}/opt/mawsh/examples/
%files
        %{_bindir}/mawd
        %{_bindir}/mawsh
        %{_sysconfdir}/sysconfig/mawd.conf
        /usr/lib/systemd/system/mawd.service
        %{_includedir}/mawsh/*.cl
        /opt/mawsh

%preun
        %systemd_preun %{name}.service

%postun
        %systemd_postun_with_restart %{name}.service

%clean
    rm -rf %{buildroot}

%changelog
