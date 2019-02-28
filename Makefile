EXECUTABLE_OUTPUT_PATH=/usr/bin/
DAEMON_DIR=/etc/systemd/system/

PHONY: all
all: bin daemon

bin:
		gcc src/daemon.c -o $(EXECUTABLE_OUTPUT_PATH)/daemon

daemon:
		cp WebServer.service $(DAEMON_DIR)


