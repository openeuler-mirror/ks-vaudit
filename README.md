-ks-vaudit


编译安装
    1. 安装编译依赖
        yum install ffmpeg-devel cmake3 qt5-qtx11extras-devel qt5-qtbase-devel qt5-linguist libXext-devel libXfixes-devel libXinerama-devel libXi-devel cairo-devel

    2. 进入 ks-vaudit 目录, 执行如下编译命令:
        ENABLE_32BIT_GLINJECT=FALSE ./simple-build-and-install 命令

安装
    在build-release 目录下执行sudo make install

运行
    编译安装运行后执行 ks-vaudit


后端架构
        +---------------+                            +--------------+
        |   X11Input    |      +--------------+      |              |      +-------+
        |      or       | ---> |              | ===> | VideoEncoder | ===> |       |
        | GLInjectInput |      |              |      |              |      |       |
        +---------------+      | Synchronizer |      +--------------+      | Muxer |
        |               |      |              |      |              |      |       |
        |   ALSAInput   | ---> |              | ===> | AudioEncoder | ===> |       |
        |               |      +--------------+      |              |      +-------+
        +---------------+                            +--------------+

    说明：
        ALSAInput、 X11Input 捕获音频数据及Xserver 数据帧
        Synchronizer 表示音视频同步处理
        VideoEncoder AudioEncoder 音视频编码处理
        Muxer 音视频合成器
