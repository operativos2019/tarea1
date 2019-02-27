systemctl stop simple-daemon
systemctl disable simple-daemon
rm -rf build
rm /etc/systemd/system/simple-daemon.service
rm /usr/bin/daemon
systemctl daemon-reload
systemctl reset-failed
systemctl status simple-daemon

#/usr/lib/systemd/system/simple-daemon

#systemctl stop [servicename]
#systemctl disable [servicename]
#rm /etc/systemd/system/[servicename]
#rm /etc/systemd/system/[servicename] symlinks that might be related
