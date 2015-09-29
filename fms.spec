Name: fms
Summary: fault management system
Version: 2.0
Release: 1
Vendor: INSPUR
License: INSPUR SOFTWARE LICENSE.
URL: www.inspur.com
Group: Applications/System
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires: chkconfig
%description
INSPUR fault management system.

%prep
#%setup
#%patch

%build
%install
	cp -r %{_builddir}/%{name}/* $RPM_BUILD_ROOT

%clean
	rm -rf $RPM_BUILD_ROOT

%pre
%post
	/bin/chmod 755 /usr/sbin/fmd

%preun

%files

%defattr(-,root,root)
/usr/sbin/*
/var/log/*
#/etc/*
/lib/*
/usr/lib/fms/*

%changelog