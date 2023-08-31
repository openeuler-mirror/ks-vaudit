#/bin/bash
export KS_PATH="$1"
export PROJECK_NAME="$2"

if [ "$KS_PATH" == "" ]; then
        echo "please check projeck tar is it correct!"
        echo "$KS_PATH"
        exit 1
fi

if [ "$PROJECK_NAME" == "" ]; then
        echo "please input projeck name!"
        echo "$PROJECK_NAME"
        exit 1
fi

VER_PROJECK_NAME=`ls ${KS_PATH}/${PROJECK_NAME}`

if [ "$VER_PROJECK_NAME" == "" ]; then
	echo "ks project is non-existent"
	echo "$PROJECK_NAME"
	exit 1
fi

# define os tags
TAG_30="3.0"
TAG_32="3.2"
TAG_33="3.3"
TAG_34="3.4"

# find os version
OS_VERSION=`cat /etc/.kyinfo | grep dist_id | awk -F ' ' '{ print $3 }'`

echo "OS version:${OS_VERSION}"

DEST_DIR=""

case ${OS_VERSION} in
    *${TAG_30}*)    DEST_DIR="KY3.0" ;;
    *${TAG_32}*)    DEST_DIR="KY3.2" ;;
    *${TAG_33}*)    DEST_DIR="KY3.3" ;;
    *${TAG_34}*)    DEST_DIR="KY3.4" ;;
esac

if [ "${OS_VERSION}" = "" ] || [ "${DEST_DIR}" = "" ]; then
    echo "Cannot match ${OS_VERSION}"
    echo "********************"
    echo "type  version"
    echo "1     KY3.0"
    echo "2     KY3.2"
    echo "3     KY3.3"
    echo "4     KY3.4"
    echo "please input the type of system version:"

    read INPUT_TYPE

    DEST_DIR=""
    case ${INPUT_TYPE} in
        1)    DEST_DIR="KY3.0" ;;
        2)    DEST_DIR="KY3.2" ;;
        3)    DEST_DIR="KY3.3" ;;
        4)    DEST_DIR="KY3.4" ;;
    esac

    if [ "$DEST_DIR" = "" ]; then
        echo " the input type is not valid"
	exit 1
    fi
fi

if [ -d ${KS_PATH}/license/${DEST_DIR} ]; then
    chmod +x ${KS_PATH}/license/${DEST_DIR}/install*.sh && ${KS_PATH}/license/${DEST_DIR}/install*.sh ${KS_PATH}/license/${DEST_DIR}
else
    echo "Cannot find path:${KS_PATH}/license/${DEST_DIR}"
fi

if [ -d ${KS_PATH}/${PROJECK_NAME}/${DEST_DIR} ]; then
	chmod +x ${KS_PATH}/${PROJECK_NAME}/${DEST_DIR}/install*.sh && ${KS_PATH}/${PROJECK_NAME}/${DEST_DIR}/install*.sh ${KS_PATH}/${PROJECK_NAME}/${DEST_DIR}
else
       	echo "Cannot find path:${KS_PATH}/${PROJECK_NAME}/${DEST_DIR}"
fi

