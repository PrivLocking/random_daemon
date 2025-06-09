#!/bin/bash
userName=ti
if [ "${UID}" = '0' ] ; then
    echo su - ${userName} -c -- "$0 $@"
    exec su - ${userName} -c -- "$0 $@"
    exit
fi

echo " === ${USER} ${UID} ==="
nohup /etc/haproxy/yt_random_daemon.bin -f /etc/haproxy/videos.list.txt > /tmp/log.yt_random_daemon.log &

sleep 1

