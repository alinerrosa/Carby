#!/bin/bash

PID=`cat /home/ubuntu/assistant.pid`

if ! ps -p $PID > /dev/null
then
  rm /home/ubuntu/assistant.pid
  screen -S assistant
  source env/bin/activate 
  googlesamples-assistant-pushtotalk --project-id 'raspberriassistant-600e5' --device-model-id 'raspberriassistant-600e5-carby-j63dqh' --lang pt-BR & echo $! >>/home/ubuntu/assistant.pid
fi
