# 增值业务的run包制作
该项目为生成增值业务run包的项目，包括生成脚本以及rpm包，已提高后续增值业务run包制作统一管理与工作效率。

## run包生成步骤

使用 ./generate-run.sh [KS_PROJECK_NAME] [KS_PROJECK_VERSION] 生成run包（如./generate-run.sh ks-ssr 1.0）

```
1，在ks-run目录下新建增值业务（例如mkdir ks-run/ks-ssr）

2，创建架构目录（KY3.0/KY3.2/KY3.3/KY3.4）（例如mkdir ks-run/ks-ssr/KY3.3）（后续增值业务可能会用.so的方式打包，
不区分架构可创建KY3.X目录,可在ks-run/install.sh中删除${DEST_DIR}）

3，在ks-run/[KS_PROJECK_NAME]/KY3.*中添加install-[KS_PROJECK_NAME].sh安装脚本与x86_64/aarch64目录存放rpm包（如 ks-run/ks-ssr/KY3.3/install-ks-ssr.sh ks-run/ks-ssr/KY3.3/x86_64）

4，执行generate-run.sh
```
## 注意事项

```
1，生成run包的命名为[KS_PROJECK_NAME] + [KS_PROJECK_VERSION] + 当前时间（年月日）+ .run，run包安装时也会去[KS_PROJECK_NAME]的对应目录下找，所以
使用generate-run.sh 必须加[KS_PROJECK_NAME] [KS_PROJECK_VERSION] 这两个参数。
2，run包会先升级kylin-license后安装增值软件，kylin-license只会安装一次，检测到系统license版本等于或高于run包中的版本，则不做安装。
