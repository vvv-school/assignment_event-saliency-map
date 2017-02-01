#!/bin/bash

# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini <ugo.pattacini@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

# Put here those instructions we need to execute before running the test

yarp where
if [ $? -eq 0 ]; then
   kill_yarp="no"
else
   kill_yarp="yes"
   yarpserver --write &
   sleep 1
fi

ln -s ../../app/scripts/app_event-saliency-map.xml fixtures/fixture.xml

yarpdataplayer &
sleep 1
echo "load $ROBOT_CODE/datasets/Dataset_event-saliency-map" | yarp rpc /yarpdataplayer/rpc:i
sleep 1
