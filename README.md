-ks-vaudit
-====================

编译安装

    1. 安装devtoolset-8-gcc 相关软件包

    2. 安装qt-opensource-linux-x64-5.7.1.run  QT5.7.1 版本

    3. 配置PKG_CONFIG_PATH,PATH 环境变量内容如下:
        export PKG_CONFIG_PATH=/opt/Qt5.7.1/5.7/gcc_64/lib/pkgconfig:$PKG_CONFIG_PATH
        export PATH=/opt/Qt5.7.1/5.7/gcc_64/bin:$PATH

    4. 安装编译依赖
        yum install ffmpeg-devel cmake3 libv4l-devel jack-audio-connection-kit-devel

    5. 执行如下编译命令:
        ENABLE_32BIT_GLINJECT=FALSE ./simple-build-and-install 命令


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

