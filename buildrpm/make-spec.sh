#!/bin/bash

ls -l ~/rpmbuild/SOURCES/ > /dev/null 2>&1 || mkdir -p ~/rpmbuild/SOURCES/
ls -l ~/rpmbuild/SPECS/ > /dev/null 2>&1 || mkdir -p ~/rpmbuild/SPECS/
rpm -qa | grep rpm-build > /dev/null 2>&1 || sudo yum -y install rpm-build

cp ks-vaudit.spec ~/rpmbuild/SPECS/
cd ../
tar -zcf ~/rpmbuild/SOURCES/ks-vaudit-1.0.0.tar.gz .

cd ~/rpmbuild/SPECS/
sudo yum-builddep ks-vaudit.spec

rpmbuild -ba ks-vaudit.spec
