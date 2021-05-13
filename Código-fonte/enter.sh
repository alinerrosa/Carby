#!/bin/bash

PID=`cat /home/ubuntu/assistant.pid`

if ps -p $PID > /dev/null
then
  for i in {1..58}
  do
    screen -S assistant -p 0 -X echo
    sleep 1
  done
fi
