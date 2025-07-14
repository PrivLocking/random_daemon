#!/bin/bash
userName=ti
realBin=`realpath $0`
if [ "${UID}" = '0' ] ; then
    echo su - ${userName} -c -- "${realBin} $@"
    exec su - ${userName} -c -- "${realBin} $@"
    exit
fi

baseName=yt_random_daemon.bin
if [ "$1" = 'stop' ] ; then
    echo "stoping... ${0} : ${baseName}"
    echo "exit...1"
    killall  ${baseName}
    echo "exit...2"
    pkill     ${baseName} 2>/dev/null
    pkill -f  ${baseName} 2>/dev/null
    echo "exit...3"
    exit
fi

patH1=/etc/haproxy
patH1=/etc/nginx/sites-enabled/run
patH2=/v3T/bible/wwwJ3
patH9=/tmp

echo " === ${USER} ${UID} ==="
rm -f                                        ${patH9}/yt_random_daemon.bin 
cat       ${patH1}/yt_random_daemon.bin    > ${patH9}/yt_random_daemon.bin  
chmod 755                                    ${patH9}/yt_random_daemon.bin 
md5sum                                       ${patH9}/yt_random_daemon.bin 

md5sum                                       ${patH9}/yt_random_daemon.bin                         > /tmp/log.yt_random_daemon.log 
nohup     ${patH9}/yt_random_daemon.bin -f ${patH2}/videos.list1.txt -2 ${patH2}/videos.list2.txt >> /tmp/log.yt_random_daemon.log &

sleep 1
echo 'cat /tmp/log.yt_random_daemon.log'
      cat /tmp/log.yt_random_daemon.log



