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

sudo cat > /etc/ld.so.conf.d/ks-vaudit-x86_64.conf <<EOF
/usr/local/lib64
/usr/local/ks-vaudit/Qt5.7.1/5.7/gcc_64/lib
EOF

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
install_package "qrencode-libs"
install_package "postgresql-libs"
install_package "postgresql-8.4.20"
install_package "qrencode-3.4.2"
install_package "kiran-log-qt5"
install_package "freeglut"
install_package "unixODBC"
sudo rpm -Uvh $RPM_PATH/ks-vaudit* --nodeps

sudo chmod u+s /usr/bin/ks-vaudit
#sudo chmod u+s /usr/bin/ks-vaudit-record
#sudo chmod u+s /usr/bin/ks-vaudit-audit

sudo cat > /tmp/ks-vaudit-uninstall <<EOF
sudo rpm -e ks-vaudit --nodeps
sudo rm -rf /usr/local/ks-vaudit
sudo rm -rf /etc/ld.so.conf.d/ks-vaudit-x86_64.conf
sudo rm -rf /usr/bin/ks-vaudit-uninstall
EOF
sudo mv /tmp/ks-vaudit-uninstall /usr/bin/ks-vaudit-uninstall && sudo chmod 0755 /usr/bin/ks-vaudit-uninstall

# source /etc/bashrc
# sudo service ks-vaudit-configure restart
# sudo service ks-vaudit-monitor restart