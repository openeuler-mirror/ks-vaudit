#!/bin/bash

# check the os version
VERSION_KY33=`cat  /etc/.kyinfo  | grep "milestone = 3.2"`
if [ "${VERSION_KY33}" == "" ]; then
    echo "WARNING: This only can install on kylinsec 3.2"
    exit 1
fi

if [ -d "$1" ]; then
    CURR_PATH=$1
else
    CURR_PATH=`pwd`
fi

# find os version
ARCH_TYPE=`cat /etc/.kyinfo | grep "arch =" | awk -F ' ' '{ print $3 }'`

if [ "${ARCH_TYPE}" ]; then
    echo "Current arch is ${ARCH_TYPE}"
    RPM_PATH=${CURR_PATH}/${ARCH_TYPE}
else
    echo "Cannot find arch"
    exit 1
fi

if [ -d "${RPM_PATH}" ]; then
    echo "The rpm path ${RPM_PATH}"
else
    echo "Cannot find path ${RPM_PATH}"
    exit 1
fi

function install_package()
{
    RPM_NAME=$1
    rpm -qa | grep ${RPM_NAME} > /dev/null || sudo rpm -Uvh ${RPM_PATH}/${RPM_NAME}*.rpm --nodeps
}

install_package "libva-2.3"
install_package "gmmlib"
install_package "media-driver"
install_package "SDL2"
install_package "msdk"
install_package "libvpx"
install_package "x264-libs"
install_package "ffmpeg"
install_package "libva-utils"
install_package "intel-vaapi-driver"
install_package "nv-codec-headers"
install_package "qt5-qtbase-5.6.1"
install_package "qt5-qtbase-common"
install_package "qt5-qtbase-gui"
install_package "QtCipherSqlitePlugin"
install_package "qt5-qtx11extras"
install_package "qt5-qtmultimedia"
install_package "qrencode-libs"
install_package "postgresql-libs"
install_package "postgresql-8.4.20"
install_package "qrencode-3.4.2"
install_package "kiran-log-qt5"
install_package "freeglut"
install_package "libvdpau"
install_package "unixODBC"
sudo rpm -Uvh $RPM_PATH/ks-vaudit* --nodeps --force

sudo chmod u+s /usr/bin/ks-vaudit

sudo cat > /tmp/ks-vaudit-uninstall <<EOF
sudo rpm -e ks-vaudit --nodeps
sudo rm -rf /usr/local/ks-vaudit
sudo rm -rf /usr/bin/ks-vaudit-uninstall
EOF
sudo mv /tmp/ks-vaudit-uninstall /usr/bin/ks-vaudit-uninstall && sudo chmod 0755 /usr/bin/ks-vaudit-uninstall
