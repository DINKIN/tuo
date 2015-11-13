Print Messages

Use printk to print messages in kernel mode, for example

> printk( KERN\_INFO, "%s installed, device major number %d\n", DEVICE\_NAME, major);

The KERN\_INFO specifies the level of message. Kernel message levels are defined in linux/kernel.h
```
#define	KERN_EMERG	"<0>"	/* system is unusable			*/
#define	KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define	KERN_CRIT	"<2>"	/* critical conditions			*/
#define	KERN_ERR	"<3>"	/* error conditions			*/
#define	KERN_WARNING	"<4>"	/* warning conditions			*/
#define	KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define	KERN_INFO	"<6>"	/* informational			*/
#define	KERN_DEBUG	"<7>"	/* debug-level messages			*/
```
Generally, messages will be log into /usr/adm/messages,/usr/adm/debug, and /usr/adm/syslog (specified in /etc/syslog.conf). It is a good idea to use command tail -f /usr/adm/messages to monitor printed messages, however, you must own root privilege to do that.

Message that priority is higher then KERN\_ERR (not included, klogd -c 3) will be direct to system console but X terminal, i.e., if you like to see the critial messages from kernel (modules,) do not load modules within a X terminal. If you insist to do that (display message on X terminal,) write a new printk.

/proc/sys/kernel/printk
Here's the section of the proc man page that pretends to explain it: The  four  values  in  the  file  printk  are  console\_loglevel, default\_message\_loglevel, minimum\_console\_level and default\_console\_loglevel.  These values influence  printk()  behavior when printing  or logging error messages. See syslog(2) for more info on the different loglevels.  Messages  with  a  higher priority than  console\_loglevel will be printed to the console. Messages               without an explicit  priority  will  be  printed  with priority default\_message\_level. minimum\_console\_loglevel is the minimum (highest)  value  to  which   console\_loglevel can   be set. default\_console\_loglevel   is   the   default   value  for console\_loglevel.

Read manual of klogd (man klogd) for more information.