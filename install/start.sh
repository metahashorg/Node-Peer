#!/bin/bash

CONF_DIR=$(pwd)
APP="metahash/peer_node:latest"
NAME="peer_node"

sudo docker pull $APP
sudo docker stop $NAME
sudo docker rm $NAME
sudo docker run --restart always --net=host -d --name $NAME -v ${CONF_DIR}/:/conf $APP peer.conf peer.network private.key
sudo docker logs $NAME
