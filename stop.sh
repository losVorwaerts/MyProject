#!/bin/sh

sudo /usr/local/nginx/sbin/nginx -s stop

sudo /usr/bin/stop.sh /etc/fdfs/tracker.conf
sudo /usr/bin/stop.sh /etc/fdfs/storage.conf

sudo redis-cli shutdown

sudo server mysqld stop
