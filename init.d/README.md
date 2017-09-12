The `init.d` directory includes the template script that allows to run Jool as a service running from boot. I recommend to create a service for each Jool mode (NAT64 and SIIT)

# How to configure Jool/Jool SIIT init.d service

1. Copy the jool/jool_siit file to `/etc/init.d/`. The file should look something like this:

        #!/bin/bash
		#chkconfig: 2345 20 80

		# Source function library
		. /etc/init.d/functions

		start(){
			/sbin/modprobe jool pool6=65:ff9b::/96
		}


		stop(){
			/sbin/modprobe -r jool
		}


		case "$1" in
			start)
				start
				;;
			stop)
				stop
				;;
			restart)
				start
				stop
				;;
			status)
				jool
				;;
			*)	

		echo "Usage: $0 {start|stop|status|restart}"

		esac

		exit 0

		
2. Replace the `start()` code with any configuration/commands required for your Jool's configuration. For example:

		start(){
			/sbin/modprobe jool
			/usr/local/bin/jool --file /etc/jool.conf
		}
	
	where jool.conf is a json file including atomic configuration:
	
		{
			"pool6": "64:ff9b::/96"
		}
	
If you want to use the atomic configuration, you can use the example files jool.conf and jool_siit.conf and edit the file path in the start() code.
	
	
3. Replace the `stop()` code with any commands required to stop Jool. For example:
	
		stop(){
			/sbin/modprobe -r jool
		}
	
4. Replace the `restart()` code with any commands required to stop Jool. It is recommended to include a call to stop() and start(). 


5. Replace the code inside `status)` case with any commands required to show the status for Jool service. For example: 

		status)
			/usr/local/bin/jool 
			;;
			
			
6. Mark the scripts as executables

		chmod +x jool
		chmod +x jool_siit

7. Add the script to init.d running the following commands: 
	CentOS/RHEL:
		chkconfig --add jool 
		chkconfig --add jool_siit
		chkconfig --level 2345 jool on
		chkconfig --level 2345 jool_siit on
		
	Debian/Ubuntu
		update-rc.d jool defaults
		update-rc.d jool_siit defaults
		update-rc.d jool start 20 2 3 4 5
		update-rc.d jool_siit start 20 2 3 4 5
	
8. Reboot and check if Jool was configured.