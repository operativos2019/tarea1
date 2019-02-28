systemctl stop WebServer.service
systemctl disable WebServer.service
rm -rf build
rm /etc/systemd/system/WebServer.service
rm /usr/bin/daemon
systemctl daemon-reload
systemctl reset-failed
systemctl status WebServer.service

#/usr/lib/systemd/system/simple-daemon

#systemctl stop [servicename]
#systemctl disable [servicename]
#rm /etc/systemd/system/[servicename]
#rm /etc/systemd/system/[servicename] symlinks that might be related
