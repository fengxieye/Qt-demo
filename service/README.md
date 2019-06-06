使用window service打开外部exe的demo

test_service 是工程文件

release下是编译好的可以测试效果的文件，运行服务打开一个窗口程序
使用方式：
1.直接运行 MyService -install 安装运行服务
           MyService -uninstall 卸载服务
		   
   因为代码里面写了安装服务
   https://github.com/lgxZJ/Miscellaneous/blob/master/WinService/Source.cpp
   
2. 使用cmd命令：
sc create xxx binpath= D:\...\...\MyService.exe
确认在“binPath=”后，复制了空格。

sc start xxx

sc stop xxx

sc restart xxx

sc  delete xxx


如果出现：
[SC] OpenSCManager FAILED 5:
https://blog.csdn.net/rominsoft/article/details/20954133
权限问题，可以管理员的方式来启动就好


服务查看里面，删除后f5刷新才显示消失