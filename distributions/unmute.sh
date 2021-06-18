!/bin/bash

OPUS="opus-1.3.1"
NCORES=$(nproc)
ADEVICE=$(aplay -l|grep "USB Audio"|tail -1|cut -d' ' -f3)
export LD_LIBRARY_PATH="distributions/${OPUS}/.libs:distributions/jack2/build:distributions/jack2/build/common"

cd ..
distributions/jack2/build/jackd -R -T --silent -P70 -p16 -t2000 -d alsa -dhw:${ADEVICE} -p 128 -n 3 -r 48000 -s &
./Jamulus --username DefaultUsername

killall Jamulus
killall jackd

