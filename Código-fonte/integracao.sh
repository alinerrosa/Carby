#!/bin/bash

PID=`cat /home/ubuntu/dados.pid`

if ! ps -p $PID > /dev/null
then
  rm /home/ubuntu/dados.pid
  sudo ./mcp3008hwspi & echo $! >>/home/ubuntu/dados.pid
fi
