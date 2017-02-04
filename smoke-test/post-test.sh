#!/bin/bash

# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini <ugo.pattacini@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

# Put here those instructions we need to execute after having run the test

echo "stop" | yarp rpc /yarpdataplayer/rpc:i
echo "quit" | yarp rpc /yarpdataplayer/rpc:i

if [ "${kill_yarp}" == "yes" ]; then
  killall -9 yarpserver
fi

unset kill_yarp
