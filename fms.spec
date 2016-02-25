Name: fms
Summary: fault management system
Version: 2.0
Release: 6
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
	ldconfig
	systemctl enable fmd 1>/dev/null 2>/dev/null
	
%preun
	systemctl stop fmd 1>/dev/null 2>/dev/null
	#systemctl disable fmd 1>/dev/null 2>/dev/null
	
	lsmod | grep kfm >/dev/null
	if [ $? -eq 0 ] ; then
	echo 1 > /proc/sys/fm/unload 
	rmmod kfm
	fi

%postun
	ldconfig
	
%files

%defattr(-,root,root)
/usr/sbin/*
/var/log/*
/lib/*
/usr/lib/fms/*
/etc/ld.so.conf.d/*

%changelog
