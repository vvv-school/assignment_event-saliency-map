Event-driven saliency map
=============================

Create an event-driven saliency map based on the output of the feature maps created with oriented filters

# Prerequisites
By now, you should know enough about event-driven feature maps ([Orientation Filter](https://github.com/vvv-school/solution_event-orientation-filter)) and _yarpmanager scripts_ (e.g. (https://github.com/vvv-school/solution_event-orientation-filter)).

# Assignment
You have to implement a hierarchical model of selective attention, based on the feature maps output developed in the previous assignment.
 
To accomplish this task you have to modify the [app](./app/scripts) to instantiate multiple oriented filter maps, each taking as input the event stream from the cameras and sending their output bottles to the [Selective Attention](https://github.com/vvv-school/solution_event-saliency-map/tree/master/src) module. This module has been completed for you, for the feature map modules you should use the code you developed in the [Orientation Filter] assignment, if it is not working you can use the solution provided (https://github.com/vvv-school/solution_event-orientation-filter). 
You have to correctly tune the parameters of the modules (tau, Te, theta, strict, ...).

You have to visualise:

1. The output of one feature map (spiking or subthreshold)
1. The input event-stream (using [vFramer])
1. The output of the [Selective Attention Module]   (spiking and subthreshold)

You will run this assignment with the [_Dataset_event-saliency-map_]() dataset. The dataset has bars at different orientations and an object with multiple edges at different orientations. Each feature map will contribute to the energy of the region where the features superimpose and the saliency map will have maximum activity there. The centre and dimension of the maximum activity region will define a region of interest.

To get a bonus :-)

1. Find the maximum of the Saliency Map and send it using a YARP bottle (non-event type)
1. Find the size of the Region of Interest and send it.

This final work will make you proud and will be extremely useful during the last integration day.

Once done, you can test your code **Automatically**: [running the script **test.sh**](https://github.com/vvv-school/vvv-school.github.io/blob/master/instructions/how-to-run-smoke-tests.md) in the **smoke-test** directory. 

# [How to complete the assignment](https://github.com/vvv-school/vvv-school.github.io/blob/master/instructions/how-to-complete-assignments.md)
