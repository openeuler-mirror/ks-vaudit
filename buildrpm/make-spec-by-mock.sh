#!/bin/bash

set -ex

if [[ "$(id -u)" != 0 ]]; then
	echo "must be root user, mock cmd need it"
	exit 1
fi

pushd .
SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
release=$(cat ${SHELL_FOLDER}/ks-vaudit.spec | grep "Release: " | awk '{print $2}')

builddir="${SHELL_FOLDER}/buildtmp"
rm -rf $builddir
mkdir -p $builddir

resultdir="${SHELL_FOLDER}/buildres"
rm -rf $resultdir
mkdir -p $resultdir

rm -f "${builddir}/ks-vaudit-1.0.0.tar.gz"
cd ${SHELL_FOLDER}/..
git archive -o "${builddir}/ks-vaudit-1.0.0.tar.gz" HEAD
#tar zcf "${builddir}/ks-vaudit-1.0.0.tar.gz" .

rm -f "${builddir}/ks-vaudit.spec"
cp --preserve=time ${SHELL_FOLDER}/ks-vaudit.spec $builddir

cp -rf ${SHELL_FOLDER}/kylin-3-x86-64.cfg /etc/mock

cd $builddir
mock -r kylin-3-x86-64 --buildsrpm --spec ./ks-vaudit.spec --sources ./ks-vaudit-1.0.0.tar.gz --resultdir "$resultdir"
mock -r kylin-3-x86-64 --rebuild "$resultdir/ks-vaudit-1.0.0-${release}.src.rpm" --resultdir "$resultdir"
popd

