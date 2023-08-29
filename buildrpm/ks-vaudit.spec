Name:           ks-vaudit
Version:        1.0.0
Release:        1
Summary:        kylinsec vaudit

License:        GPL
Source0:        %{name}-%{version}.tar.gz
BuildRoot:	%_topdir/BUILDROOT

ExcludeArch:   i686
ExcludeArch:   armv7hl
ExcludeArch:   aarch64

BuildRequires: cmake3 >= 3.6.1
BuildRequires: devtoolset-8-binutils
BuildRequires: devtoolset-8-runtime
BuildRequires: devtoolset-8-gcc
BuildRequires: devtoolset-8-gcc-c++
BuildRequires: devtoolset-8-libstdc++-devel
BuildRequires: mesa-libGL-devel
BuildRequires: libX11-devel
BuildRequires: libXext-devel
BuildRequires: libXfixes-devel
BuildRequires: libXi-devel
BuildRequires: libXinerama-devel
BuildRequires: libpciaccess-devel
BuildRequires: pulseaudio-libs-devel
BuildRequires: cairo-devel
BuildRequires: zlog >= 1.2.15
BuildRequires: zlog-devel
BuildRequires: kiran-log-qt5-devel
BuildRequires: ffmpeg >= 4.3.4
BuildRequires: libnotify-devel
BuildRequires: kylin-license-devel
BuildRequires: mesa-libGL-devel
BuildRequires: nv-codec-headers
BuildRequires: qrencode-devel
BuildRequires: freeglut-devel

Requires:      libX11
Requires:      ffmpeg >= 4.3.4
Requires:      libnotify
Requires:      zlog >= 1.2.15
Requires:      kiran-log-qt5
Requires:      postgresql
Requires:      unixODBC
Requires:      mesa-libGL
Requires:      kylin-license-client
Requires:      kylin-license-core
Requires:      qrencode
Requires:      freeglut

%define debug_package %{nil}
%define __spec_install_post\
    %{?__debug_package:%{__debug_install_post}}\
    %{__os_install_post}\
%{nil}

%description
%{summary}.

%prep
%autosetup -c -n %{name}-%{version}

%build
set +e
source scl_source enable devtoolset-8
qtpath=`pwd`/buildrpm
tar -xf ${qtpath}/Qt5.7.1.tar.gz -C ${qtpath}
qtpath=${qtpath}/ks-vaudit/Qt5.7.1/5.7/gcc_64
libpath=${qtpath}/lib
export PATH="${qtpath}/bin:/usr/local/bin:$PATH"
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$libpath/pkgconfig/:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/lib64:/usr/local/lib:/lib:/lib64:/usr/lib64:/usr/lib:$libpath:$LD_LIBRARY_PATH
export LIBVA_DRIVERS_PATH=/usr/local/lib64/dri
export LIBVA_DRIVER_NAME=iHD

ls -l /usr/bin/cmake > /dev/null 2>&1 || sudo ln -s /usr/bin/cmake3 /usr/bin/cmake
bash simple-build-and-install

%if %{with tests}
%check
make check
%endif

%install
qtpath=%_topdir/BUILD/%{name}-%{version}/buildrpm/ks-vaudit
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/
cp ${qtpath} %{buildroot}/usr/local/ -rf
cd ./build-release
%make_install
mkdir -p %{buildroot}/usr/bin/
install ./src/ui/ks-vaudit-record/src/ks-vaudit-record %{buildroot}/usr/bin/
sudo chmod +x %{buildroot}/etc/init.d/ks-vaudit-configure

%files
/usr/local/ks-vaudit/Qt5.7.1
/etc/init.d/*
/etc/dbus-1/system.d/*
/usr/bin/*
#simple-build-and-install PREFIX="xxxx"
/usr/share/*

%post
ldconfig
qtlib=/usr/local/ks-vaudit/Qt5.7.1/5.7/gcc_64/lib
cat /etc/bashrc | grep "export LIBVA_DRIVERS_PATH=" > /dev/null 2>&1 || echo "export LIBVA_DRIVERS_PATH=/usr/local/lib64/dri" >> /etc/bashrc
cat /etc/bashrc | grep "export PKG_CONFIG_PATH=" > /dev/null 2>&1 || echo "export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:${qtlib}/pkgconfig:\$PKG_CONFIG_PATH" >> /etc/bashrc
cat /etc/bashrc | grep "export LD_LIBRARY_PATH=" > /dev/null 2>&1 || echo "export LD_LIBRARY_PATH=/usr/local/lib64:/lib64:${qtlib}:\$LD_LIBRARY_PATH" >> /etc/bashrc 

sudo service ks-vaudit-configure restart
sudo chkconfig ks-vaudit-configure on
sudo service ks-vaudit-monitor restart
sudo chkconfig ks-vaudit-monitor on

%preun
sudo service ks-vaudit-configure stop
sudo service ks-vaudit-monitor stop
sed -i -e '/^export LIBVA_DRIVERS_PATH=/,+2d' /etc/bashrc

%clean
qtpath=%_topdir/BUILD/Qt5.7.1
rm -rf $RPM_BUILD_ROOT ${qtpath}

%changelog
* Mon Jun 27 2022 zhenggongping <zhenggongping@kylinsec.com.cn> - 1.0.0-1
- KYOS-F: first release
