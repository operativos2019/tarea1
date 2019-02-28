systemctl stop WebServer.service
systemctl disable WebServer.service
rm /etc/systemd/system/WebServer.service
rm /usr/bin/daemon
systemctl daemon-reload
systemctl reset-failed
systemctl status WebServer.service
