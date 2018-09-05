#include "event-orientation-filter.h"
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
    return spikingmodel.initialise(rf.check("name", yarp::os::Value("/vSpikingModel")).asString(),
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

    initialiseFilter(filterSize, theta);

    debugPrintFilter();

    return true;
}

void spikingModel::initialiseFilter(int filterSize, int theta)
{
    //calculate the parameters of the line given theta
    //ax + by + c = 0

    double a = cos(theta * M_PI / 180.0);
    double b = sin(theta * M_PI /180.0);
    double c = (a*(filterSize-1) + b*(filterSize-1))/(-2.0);

    filter.resize(filterSize, filterSize);
    filter.zero();
    for(int yi = 0; yi < filterSize; yi++) {
        for(int xi = 0; xi < filterSize; xi++) {
            double d = a * xi + b * yi + c;
            if(fabs(d) < 0.5) {
            //if(d > 0 && d < 1.0) {
                filter(xi, yi) = 1.0;
            }
        }
    }
}

void spikingModel::debugPrintFilter()
{
    for(int yi = 0; yi < filtersize; yi++) {
        for(int xi = 0; xi < filtersize; xi++) {
            std::cout << filter(xi, yi) << " ";
        }
        std::cout << std::endl;
    }

}

void spikingModel::updateModelUsingFilter(int x, int y, int ts)
{
    for(int yi = 0; yi < filtersize; yi++) {
        for(int xi = 0; xi < filtersize; xi++) {
            if(filter(xi, yi))
                updateModel(x + xi, y + yi, ts, filter(xi, yi));
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
    float &energy = energymap(x + filtersize/2, y + filtersize/2);
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

        updateModelUsingFilter(v->x, v->y, v->stamp);

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
        int currentspiketime = q.back()->stamp;
        yarp::sig::ImageOf< yarp::sig::PixelMono > &img = debugPort.prepare();
        img.resize(energymap.width(), energymap.height());
        for(int y = 0; y < energymap.height(); y++) {
            for(int x = 0; x < energymap.width(); x++) {
                updateModel(x, y, currentspiketime);
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
