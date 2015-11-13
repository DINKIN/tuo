FreeRadius的官方网站：

http://www.freeradius.org

还有一个很奇怪的http://www.freeradius.net ，提供Windows版本的下载（木马？？？）。
下面分别介绍FreeRadius在Linux和Windows上的安装过程，以及与MySQL的集成。

# phpmyprepaid #
http://sourceforge.net/projects/phpmyprepaid

# Linux下的FreeRadius #

这里安装MySQL的过程略去。

## 编译安装 ##

  * 从http://www.freeradius.org 下载最新版本（这里是2.0.5，Linux是Fedora 8）的tar包，解压缩。
  * 运行`./configure |grep mysql_config`，如果没有输出或者提示没有mysql的支持，那么需要安装mysql\_devel包，运行：`yum install "mysql_devel"`
  * 安装完`mysql_devel`后，运行`./configure; make; make install` 完成安装。
  * 如果不指定prefix，默认安装到`/usr/local`下。

## 配置 ##

配置文件在`/usr/local/etc/raddb`下。

### MySQL 配置 ###

> 在MySQL中创建需要的表。进入sql/mysql目录，运行如下命令：
```
  #] mysql -u root -p 
  mysql> create database 
  mysql> exit
  #] mysql -uroot -prootpass radius < schema.sql
  #] mysql -uroot -prootpass radius < nas.sql
  #] mysql -uroot -prootpass radius < admin.sql
```
注意admin.sql中创建了radius数据库用户和密码，并对radius用户进行了授权。
后面如果使用daloRadius，需要radius用户对radius数据库的其他表也拥有权限。因此，这里可以顺便修改admin.sql，把授权语句改成` grant all privileges on radius.* to 'radius'@'%';`

如果使用daloRadius需要进入aaa/contrib/db，用mysql-daloradius.sql在前面创建的radius数据库添加表。

### Radius配置 ###

  * 编辑`site_enabled/default`文件，把authorize, accounting, session, postauth这几个section里的sql注释去掉。
  * 编辑sql.conf里，设置正确的数据库主机、数据库用户名、密码。
  * 编辑clients.conf，设置允许任何NAS访问Radius服务器。在文件最后添加下面一段：
```
    client 0.0.0.0/1 {
        secret = testing123
        shortname = all-net
    }
```
> 然后就可以启动radius了：radiusd -X


# Windows下的FreeRadius #

与Linux主要区别源于FreeRadius的版本差异。这里是1.1.7版，Windows Server 2003 Professional。
MySQL的配置基本同上，但是Windows版本在`share\doc\freeradius\examples`提供了mysql.sql文件，用来创建radius数据库的表。

从http://www.freeradius.net 下载安装程序，安装到c:\freeradius.net目录。然后修改etc/raddb下的radiusd.con文件。找到authorize, accounting, session这几个section，把sql的注释去掉。与上面同样修改sql.conf和clients.conf文件。

**注意**：这里一定要用支持unix/dos转换格式的编辑器修改，例如UltraEdit。如果用notepad/wordpad之类修改将导致文件格式混乱。

**注意2**：记着修改user.conf，把里面的Auth-Type = System两行注释掉。否则后面验证会出错。

之后，在cmd下运行`start_radiusd_debug`启动radiusd服务。


# daloRadius的安装配置 #

  * 下载后解开到Web主目录的aaa目录下，进入library目录，修改daloradius.conf文件，填写正确的数据库名称、用户名和密码。

  * 进入aaa/contrib/db，用mysql-daloradius.sql在前面创建的radius数据库添加表。

  * 访问aaa/index.php，出现登录页面，如果登录后出错或者什么也不显示，则可能是PEAR没装。参见下面的链接完成安装，其中一定要安装DB包：
> > http://pear.php.net/manual/en/installation.shared.php

> 安装完成后，根据PAER的安装信息，修改aaa/library/opendb.php文件，修改DB.php的真实路径（最好用全路径）。
> 之后，应该就可以登录daloradius了。