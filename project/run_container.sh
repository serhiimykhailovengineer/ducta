#!/bin/bash

# set docker image name
export DOCKER_IMAGE_NAME=ducta_linux
export DOCKER_IMAGE_VERSION=0.1
export DOCKER_IMAGE_TAG=$DOCKER_IMAGE_NAME:$DOCKER_IMAGE_VERSION

DOCKER_WORKSPACE=/home/user/workspace
LOCAL_DISPLAY=docker.for.mac.host.internal:0
LOCAL_AUTORITY=~/.Xauthority
REMOTE_AUTORITY=/root/.Xauthority
XSOCK=/tmp/.X11-unix

# build docker if not exists
if [ "$(docker images -q $DOCKER_IMAGE_NAME 2> /dev/null)" == "" ]; then
  docker build -t $DOCKER_IMAGE_NAME .
fi

# Start XQuartz (ensure it's running)
open -a XQuartz

# Allow connections from localhost (XQuartz)
xhost + 127.0.0.1

# run docker with host network and device access to hackrf
# and x11 forwarding for gnuradio-companion
export EXTRA_DOCKER_ARGS="-e DISPLAY=$LOCAL_DISPLAY -v $LOCAL_AUTORITY:$REMOTE_AUTORITY -v $XSOCK:$XSOCK"
docker run -it --rm \
    -v $(pwd):/workspace \
    --net=host \
    $EXTRA_DOCKER_ARGS \
    --privileged \
    --device /dev/bus/usb \
    $DOCKER_IMAGE_NAME
