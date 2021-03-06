#include "event-saliency-map.h"
#include <math.h>

/******************************************************************************/
//main
/******************************************************************************/

int main(int argc, char * argv[])
{
    /* initialize yarp network */
    yarp::os::Network yarp;

    /* prepare and configure the resource finder */
    yarp::os::ResourceFinder rf;
    rf.setDefaultConfigFile("spikingModel.ini");
    rf.setDefaultContext("eventdriven");
    rf.configure(argc, argv);

    /* instantiate the module */
    spikingConfiguration mymodule;
    return mymodule.runModule(rf);
}

/******************************************************************************/
//spikingConfiguration
/******************************************************************************/
bool spikingConfiguration::configure(yarp::os::ResourceFinder &rf)
{
    return spikingmodel.initialise(rf.check("name", yarp::os::Value("/vSaliencyMap")).asString(),
                                   rf.check("strict", yarp::os::Value(false)).asBool(),
                                   rf.check("height", yarp::os::Value(304)).asInt(),
                                   rf.check("width", yarp::os::Value(304)).asInt(),
                                   rf.check("tau", yarp::os::Value(100000)).asDouble(),
                                   rf.check("Te", yarp::os::Value(10)).asDouble(),
                                   rf.check("filterSize", yarp::os::Value(5)).asDouble(),
                                   rf.check("theta", yarp::os::Value(0)).asDouble());
}

/******************************************************************************/
//spikingModel
/******************************************************************************/
bool spikingModel::initialise(std::string name, bool strict, unsigned int height, unsigned int width, double tau, double Te, int filterSize, int theta)
{
    if(tau < 0) {
        std::cout << "tau must be > 0" << std::endl;
        return false;
    }
    this->tau = tau;

    if(Te < 0) {
        std::cout << "Te must be > 0" << std::endl;
        return false;
    }
    this->Te = Te;

    if(filterSize < 5) {
        std::cout << "filtersize should be larger than 5" << std::endl;
        return false;
    }
    this->filtersize = filterSize;

    if(theta < 0 || theta >= 180) {
        std::cout << "orientation should be greater (or equal) to 0 and less than 180" << std::endl;
        return false;
    }
    //we don't store theta as a class variable

    this->strict = strict;
    if(strict) {
        std::cout << "Setting " << name << " to strict" << std::endl;
        setStrict();
    }

    this->useCallback();
    if(!open(name + "/vBottle:i"))
        return false;
    if(!outputPort.open(name + "/vBottle:o"))
        return false;
    if(!debugPort.open(name + "/subthreshold:o"))
        return false;

    energymap.resize(width+filtersize, height+filtersize);
    energymap.zero();

    timemap.resize(width+filtersize, height+filtersize);
    timemap.zero();

    starttime = yarp::os::Time::now();

    return true;
}

void spikingModel::updateModelUsingFilter(int x, int y, int ts)
{
    for(int yi = 0; yi < filtersize; yi++) {
        for(int xi = 0; xi < filtersize; xi++) {
            updateModel(x + xi, y + yi, ts, 1.0);
        }
    }
}

void spikingModel::updateModel(int x, int y, int ts, double inj)
{
    //get reference to the correct position
    float &energy = energymap(x, y);
    int &timestamp = timemap(x, y);

    //account for timewraps
    if(timestamp > ts) timestamp -= ev::vtsHelper::maxStamp();

    //updating the energy and timestamp
    energy *= exp(-(ts - timestamp) / tau);
    energy += inj;
    timestamp = ts;

}

bool spikingModel::spikeAndReset(int x, int y)
{
    float &energy = energymap(x, y);
    if(energy > Te) {
        energy = 0;
        return true;
    }
    return false;

}

void spikingModel::onRead(vBottle &input)
{

    //get any envelope to pass through to the output port
    yarp::os::Stamp yarpstamp;
    getEnvelope(yarpstamp);

    //prepare an output bottle if necessary
    vBottle &outputBottle = outputPort.prepare();
    outputBottle.clear();

    //get a packet of events
    vQueue q = input.get<AddressEvent>();
    if(q.empty()) return;

    //and iterate through each one
    for(vQueue::iterator qi = q.begin(); qi != q.end(); qi++) {
        event<AddressEvent> v = as_event<AddressEvent>(*qi);

        updateModelUsingFilter(v->x, v->y, (int)((yarp::os::Time::now()-starttime) * 1000000));

        //and creating a spiking event if needed
        if(this->spikeAndReset(v->x, v->y))
            outputBottle.addEvent(*qi);

    }

    //if a spike occured send on the output bottle
    if(outputBottle.size() && outputPort.getOutputCount()) {
        outputPort.setEnvelope(yarpstamp);
        if(strict)
            outputPort.writeStrict();
        else
            outputPort.write();
    }

    //if we are visualising the subthreshold layer create the image and send it
    if(debugPort.getOutputCount()) {

        //decay and convert all pixels
        int currentspiketime = (int)((yarp::os::Time::now()-starttime) * 1000000);
        yarp::sig::ImageOf< yarp::sig::PixelMono > &img = debugPort.prepare();
        img.resize(energymap.width(), energymap.height());
        for(int y = 0; y < energymap.height(); y++) {
            for(int x = 0; x < energymap.width(); x++) {
                updateModel(x, y, currentspiketime);
                if(energymap(x, y) > Te)
                    img(x, y) = 255.0;
                else
                    img(x, y) = (energymap(x, y) * 255.0) / Te;
            }
        }

        //output the image
        debugPort.write();
    }


}

void spikingModel::interrupt()
{
    outputPort.interrupt();
    debugPort.interrupt();
    yarp::os::BufferedPort<vBottle>::interrupt();
}

void spikingModel::close()
{
    outputPort.close();
    debugPort.close();
    yarp::os::BufferedPort<vBottle>::close();
}
