sudo make

systemctl enable WebServer.service
systemctl start WebServer.service
systemctl status WebServer.service