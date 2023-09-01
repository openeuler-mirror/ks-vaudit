Name:           ks-vaudit
Version:        1.0.0
Release:        9
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
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel
BuildRequires: qt5-qtx11extras-devel
BuildRequires: qt5-qtmultimedia-devel
BuildRequires: qt5-linguist
BuildRequires: QtCipherSqlitePlugin
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
BuildRequires: libvorbis-devel
BuildRequires: libtheora-devel
BuildRequires: libvdpau-devel
BuildRequires: freeglut-devel
BuildRequires: kylin-rpm-config

Requires:      qt5-qtsvg
Requires:      qt5-qtx11extras
Requires:      qt5-qtmultimedia
Requires:      QtCipherSqlitePlugin
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
Requires:      libvorbis
Requires:      libtheora
Requires:      libvpx
Requires:      libvdpau
Requires:      cpufrequtils
Requires:      freeglut

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
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/lib64:/usr/local/lib:/lib:/lib64:/usr/lib64:/usr/lib:$LD_LIBRARY_PATH
export LIBVA_DRIVERS_PATH=/usr/local/lib64/dri
export LIBVA_DRIVER_NAME=iHD

ls -l /usr/bin/cmake > /dev/null 2>&1 || ln -s /usr/bin/cmake3 /usr/bin/cmake

# bash simple-build-and-install
PREFIX="/usr"

OPTIONS=()
OPTIONS+=("-DENABLE_32BIT_GLINJECT=$ENABLE_32BIT_GLINJECT")
OPTIONS+=("-DENABLE_X86_ASM=TRUE")
OPTIONS+=("-DENABLE_FFMPEG_VERSIONS=TRUE")
OPTIONS+=("-DWITH_GLINJECT=$WITH_GLINJECT")
OPTIONS+=("-DHIGH_VERSION=$HIGH_VERSION")

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig"
export QT_SELECT="qt5"

rm -rf build-release
mkdir build-release
cd build-release

echo "Cmake ..."
# cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_BUILD_TYPE=Release "${OPTIONS[@]}" "$@" ..
cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_BUILD_TYPE=Debug "${OPTIONS[@]}" "$@" ..

echo "Compiling ..."
make -j "$( nproc )" VERBOSE=1

%if %{with tests}
%check
make check
%endif

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/
cd ./build-release
%make_install
mkdir -p %{buildroot}/usr/bin/
#install ./src/ui/ks-vaudit-record/src/ks-vaudit-record %{buildroot}/usr/bin/
chmod +x %{buildroot}/etc/init.d/ks-vaudit-configure

%files
/etc/init.d/*
/etc/dbus-1/system.d/*
/usr/bin/*
#simple-build-and-install PREFIX="xxxx"
/usr/share/*

%post
ldconfig
cat /etc/bashrc | grep "export LIBVA_DRIVERS_PATH=" > /dev/null 2>&1 || echo "export LIBVA_DRIVERS_PATH=/usr/local/lib64/dri" >> /etc/bashrc

# 配置文件存放目录
if [ ! -d "/tmp/.ks-vaudit" ];then
mkdir -p /tmp/.ks-vaudit
chmod 777 /tmp/.ks-vaudit
fi

# icons缓存刷新展示图标
gtk-update-icon-cache /usr/share/icons/hicolor/

service ks-vaudit-configure restart
chkconfig ks-vaudit-configure on
service ks-vaudit-monitor restart
chkconfig ks-vaudit-monitor on

%preun
service ks-vaudit-configure stop
service ks-vaudit-monitor stop
ps aux | grep -E "/usr/bin/ks-vaudit|ks-vaudit-audit|ks-vaudit-record$" | grep -v grep | xargs kill -2 > /dev/null 2>&1
sed -i -e '/^export LIBVA_DRIVERS_PATH=/,+d' /etc/bashrc

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Mon Oct 31 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-9
- KYOS-F: fix bugs of tests. (Related #62519)

* Sun Oct 30 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-8
- KYOS-F: fix bugs of tests. (Related #62519)

* Sun Oct 30 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-7
- KYOS-F: fix bugs of tests. (Related #62517)

* Fri Oct 28 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-6
- KYOS-F: fix bugs of tests. (Related #62481)

* Tue Oct 25 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-5
- KYOS-F: fix bugs of tests. (Related #62327)

* Mon Oct 10 2022 tangjie <tangjie@kylinsec.com.cn> - 1.0.0-4
- KYOS-F: fix bugs of tests. (Related #61683)

* Tue Aug 30 2022 zhenggongping <zhenggongping@kylinsec.com.cn> - 1.0.0-3
- KYOS-F: fix bugs of tests. (Related #60501)

* Tue Aug 30 2022 fanliwei <fanliwei@kylinsec.com.cn> - 1.0.0-2
- KYOS-F: support record mic and speaker. (Related #59481)
- KYOS-F: support nvenc. (Related #58872)
- KYOS-F: fix bugs. (Related #58873)
- KYOS-F: add audit ui. (Related #58046)

* Mon Jun 27 2022 zhenggongping <zhenggongping@kylinsec.com.cn> - 1.0.0-1
- KYOS-F: first release
