mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ../
make
sudo make install

#systemctl enable simple-daemon
systemctl start simple-daemon
systemctl status simple-daemon