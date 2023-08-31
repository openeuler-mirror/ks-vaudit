#!/bin/bash
export KS_PROJECK_NAME=$1
export KS_PROJECK_VERSION=$2

if [ "$KS_PROJECK_NAME" == "" ]; then
        echo "please input ks projeck name!"
        echo "./generate-run.sh [KS_PROJECK_NAME] [KS_PROJECK_VERSION]"
        exit 1
fi

if [ "$KS_PROJECK_VERSION" == "" ]; then
        echo "please input ks projeck version!"
        echo "./generate-run.sh [KS_PROJECK_NAME] [KS_PROJECK_VERSION]"
        exit 1
fi

CURR_PATH=`pwd`

sed -i "s/KS_PROJECK_NAME/$KS_PROJECK_NAME/g" $CURR_PATH/ks-run.sh

mkdir $CURR_PATH/ks-run-tmp
cp -r $CURR_PATH/ks-run/$KS_PROJECK_NAME $CURR_PATH/ks-run-tmp/
cp -r $CURR_PATH/ks-run/license $CURR_PATH/ks-run-tmp/
cp -r $CURR_PATH/ks-run/install.sh $CURR_PATH/ks-run-tmp/

if [ "$KS_PROJECK_NAME" == "ks-agent" ]; then
	sed -i "s/sudo //g" $CURR_PATH/ks-run-tmp/license/KY*/install-license.sh
	sed -i "s/sudo //g" $CURR_PATH/ks-run-tmp/license/KY3.3/KY3.3-x/install-license.sh
fi

tar cvf $CURR_PATH/ks-run-tmp.tar.gz -C $CURR_PATH ks-run-tmp
cat $CURR_PATH/ks-run.sh $CURR_PATH/ks-run-tmp.tar.gz > $CURR_PATH/$KS_PROJECK_NAME-$KS_PROJECK_VERSION-$(date "+%Y%m%d").run
sed -i "s/$KS_PROJECK_NAME/KS_PROJECK_NAME/g" $CURR_PATH/ks-run.sh
rm -rf $CURR_PATH/ks-run-tmp.tar.gz $CURR_PATH/ks-run-tmp
