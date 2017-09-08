The `systemd` directory includes the .service template files required by systemd in order to execute a service that configures Jool at boot. 

The systemd service allows to execute a single command (for example `/sbin/modprobe jool pool6=64:ff9b::/96`) or a whole bash script so you could include network interfaces' configuration previous to Jool. 

I recommend to create a service for each Jool mode (NAT64 and SIIT)


# How to configure Jool/Jool SIIT systemd service

1. Copy the jool.service/jool_siit.service file to `/etc/systemd/system/`. The file should look something like this:

        [Unit]
		Description=Jool NAT64 Configuration Service
		After=network.target
		
		[Service]
		Type=oneshot
		ExecStart=/sbin/modprobe jool pool6=64:ff9b::/96
		
		[Install]
		WantedBy=multi-user.target
		
2. Edit the `ExecStart` line from `Service` section in .service file according to your Jool configuration. 
	
	2.1 Single command 
	
		ExecStart=/sbin/modprobe jool pool6=64:ff9b::/96

	2.2 Bash script
	
		ExecStart=/usr/local/etc/jool-start.sh 
		
	where jool-start.sh content could be:
	
		#!/bin/bash
		sudo /sbin/modprobe jool pool6=64:ff9b::/96
		ip l set eth0 up
		ip a flush dev eth0 scope global
		ip a add 2001:db8::1/96
		... 
		
	
3. Reload the systemd daemon  

		systemctl daemon-reload

4. Start service
	
		systemctl start jool.service

		systemctl start jool_siit.service
	
5. Enable service in systemd, in order to load at boot 

		systemctl enable jool.service
	
		systemctl enable jool_siit.service

6. Reboot and check if Jool was configured.


For more information about systemd, please check the following link https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/7/html/System_Administrators_Guide/sect-Managing_Services_with_systemd-Unit_Files.html