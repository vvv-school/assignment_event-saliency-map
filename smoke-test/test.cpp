/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini <ugo.pattacini@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/
#include <string>

#include <yarp/rtf/TestCase.h>
#include <rtf/dll/Plugin.h>
#include <rtf/TestAssert.h>


#include <yarp/os/all.h>
#include <iCub/eventdriven/all.h>

using namespace std;
using namespace RTF;

class spikeChecker : public yarp::os::BufferedPort< ev::vBottle >
{

private:
    int x, y, width, height;
    int inlier;
    int outlier;

    void onRead(ev::vBottle &input) {

        ev::vQueue q = input.getAll();
        for(ev::vQueue::iterator qi = q.begin(); qi != q.end(); qi++) {
            ev::event<ev::AddressEvent> v = ev::getas<ev::AddressEvent>(*qi);
            int vx = v->getX();
            int vy = v->getY();
            if(vx >= x && vx <= x + width && vy >= y && vy <= y + height)
                inlier++;
            else
                outlier++;
        }

    }

public:

    spikeChecker() : x((304-10)/2), y((240-10)/2), width(10), height(10), inlier(0), outlier(0)
    {
        this->useCallback();
        this->setStrict();
    }

    void setROI(int x, int y, int width, int height)
    {
        this->x = x; this->y = y; this->width = width; this->height = height;
    }

    int getInliers()
    {
        return inlier;
    }

    int getOutliers()
    {
        return outlier;
    }


};


/**********************************************************************/
class TestAssignmentEventSpikingModel : public yarp::rtf::TestCase
{

private:

    spikeChecker spkchk;
    yarp::os::RpcClient playercontroller;


public:
    /******************************************************************/
    TestAssignmentEventSpikingModel() :
        yarp::rtf::TestCase("TestAssignmentEventSpikingModel")
    {
    }

    /******************************************************************/
    virtual ~TestAssignmentEventSpikingModel()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {

        //we need to load the data file into yarpdataplayer
        std::string cntlportname = "/playercontroller/rpc";

        RTF_ASSERT_ERROR_IF_FALSE(playercontroller.open(cntlportname),
                                  "Could not open RPC to yarpdataplayer");

        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect(cntlportname, "/yarpdataplayer/rpc:i"),
                                  "Could not connect RPC to yarpdataplayer");

//        //we need to check the output of yarpdataplayer is open and input of spiking model
//        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/zynqGrabber/vBottle:o", "/vOrientationFilter/vBottle:i", "udp"),
//                                  "Could not connect yarpdataplayer to spiking model");

        //check we can open our spike checking consumer
        RTF_ASSERT_ERROR_IF_FALSE(spkchk.open("/spikechecker/vBottle:i"),
                                  "Could not open spike checker");

        //the output of spiking model
        RTF_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/vSalMap/vBottle:o", "/spikechecker/vBottle:i", "udp"),
                                  "Could not connect saliency map to spike checker");

//        RTF_TEST_REPORT("Ports successfully open and connected");

        spkchk.setROI(140, 60, 105, 80);

        return true;
    }

    /******************************************************************/
    virtual void tearDown()
    {
        RTF_TEST_REPORT("Closing Clients");
        playercontroller.close();
    }

    /******************************************************************/
    virtual void run()
    {

        //play the dataset
        yarp::os::Bottle cmd, reply;
        cmd.addString("play");
        playercontroller.write(cmd, reply);
        RTF_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully play the dataset");

        yarp::os::Time::delay(5);

        cmd.clear();
        cmd.addString("stop");
        playercontroller.write(cmd, reply);
        RTF_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully stop the dataset");

        int inliers = spkchk.getInliers();
        int outliers = spkchk.getOutliers();
        RTF_TEST_REPORT(Asserter::format("Inliers = %d", inliers));
        RTF_TEST_REPORT(Asserter::format("Outliers = %d", outliers));
        RTF_ASSERT_ERROR_IF_FALSE(inliers > 7500, "Inlier score too low (7500)");
        RTF_ASSERT_ERROR_IF_FALSE(outliers < 1000, "Outlier score too high (1000)");


    }
};

PREPARE_PLUGIN(TestAssignmentEventSpikingModel)
