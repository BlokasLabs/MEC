#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <pthread.h>

#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>

#include "mec_app.h"
#include "midi_output.h"

#include <mec_api.h>
#include <mec_prefs.h>

#define OUTPUT_BUFFER_SIZE 1024

//hacks for now
#define VELOCITY 1.0f
#define PB_RANGE 2.0f
#define MPE_PB_RANGE 48.0f

class MecCmdCallback : public IMecCallback 
{
public:
    virtual void mec_control(int cmd, void* other) 
    {
        switch(cmd) {
            case IMecCallback::SHUTDOWN: {
                LOG_0( "mec requesting shutdown");
                keepRunning = 0;
                pthread_cond_broadcast(&waitCond);
                break;
            }
            default: {
                break;
            }
        }
    }
};


class MecConsoleCallback: public  MecCmdCallback
{
public:
    MecConsoleCallback(MecPreferences& p)
        :   prefs_(p),
            throttle_(p.getInt("throttle", 0)),
            valid_(true)
    {
        if (valid_) {
            LOG_0( "mecapi_proc enabling for console output, throttle :  " << throttle_);
        }
    }

    bool isValid() { return valid_;}

    void touchOn(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "touchOn";
        outputMsg(topic, touchId, note, x, y, z);
    }

    void touchContinue(int touchId, float note, float x, float y, float z)
    {
        static unsigned long count = 0;
        count++;
        static std::string topic = "touchContinue";
        //optionally display only every N continue messages
        if(throttle_==0 || (count % throttle_)== 0) {
            outputMsg(topic, touchId, note, x, y, z);
        }
    }

    void touchOff(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "touchOff";
        outputMsg(topic, touchId, note, x, y, z);

    }

    void control(int ctrlId, float v)
    {
        std::cout << "control - "
                  << " ctrlId: " << ctrlId
                  << " v:" << v
                  << std::endl;
    }

    void outputMsg(std::string topic, int touchId, float note, float x, float y, float z)
    {
        std::cout << topic << " - "
                  << " touch: " << touchId
                  << " note: " << note
                  << " x: " <<  x
                  << " y: " << y
                  << " z: " << z
                  << std::endl;
    }

private:
    MecPreferences prefs_;
    unsigned int throttle_;
    bool valid_;
};


class MecOSCCallback: public  MecCmdCallback
{
public:
    MecOSCCallback(MecPreferences& p)
        :   prefs_(p),
            transmitSocket_( IpEndpointName( p.getString("host", "127.0.0.1").c_str(), p.getInt("port", 9001) )),
            valid_(true)
    {
        if (valid_) {
            LOG_0( "mecapi_proc enabling for osc");
        }
    }

    bool isValid() { return valid_;}

    void touchOn(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "mec/touchOn";
        sendMsg(topic, touchId, note, x, y, z);
    }

    void touchContinue(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "mec/touchContinue";
        sendMsg(topic, touchId, note, x, y, z);
    }

    void touchOff(int touchId, float note, float x, float y, float z)
    {
        static std::string topic = "mec/touchOff";
        sendMsg(topic, touchId, note, x, y, z);

    }

    void control(int ctrlId, float v)
    {
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( "mec/control")
           << ctrlId << v
           << osc::EndMessage
           << osc::EndBundle;
        transmitSocket_.Send( op.Data(), op.Size() );
    }

    void sendMsg(std::string topic, int touchId, float note, float x, float y, float z)
    {
        osc::OutboundPacketStream op( buffer_, OUTPUT_BUFFER_SIZE );
        op << osc::BeginBundleImmediate
           << osc::BeginMessage( topic.c_str())
           << touchId << note << x << y << z
           << osc::EndMessage
           << osc::EndBundle;
        transmitSocket_.Send( op.Data(), op.Size() );
    }

private:
    MecPreferences prefs_;
    UdpTransmitSocket transmitSocket_;
    char buffer_[OUTPUT_BUFFER_SIZE];
    bool valid_;
};

#define GLOBAL_CH 0
#define NOTE_CH_OFFSET 1
#define BREATH_CC 2
#define STRIP_BASE_CC 0
#define PEDAL_BASE_CC 11

class MecMidiCallback: public  MecCmdCallback
{
public:
    MecMidiCallback(MecPreferences& p)
        :   prefs_(p),
            output_(p.getInt("voices", 15), (float) p.getDouble("pitchbend range", 48.0))
    {
        std::string device = prefs_.getString("device");
        int virt = prefs_.getInt("virtual", 0);
        if (output_.create(device, virt > 0)) {
            LOG_1( "MecMidiCallback enabling for midi to " << device );
            LOG_1( "TODO (MecMidiCallback) :" );
            LOG_1( "- MPE init, including PB range" );
        }
        if (!output_.isOpen()) {
            LOG_0( "MecMidiCallback not open, so invalid for" << device );
        }
    }

    bool isValid() { return output_.isOpen();}

    void touchOn(int touchId, float note, float x, float y, float z)
    {
        output_.touchOn(touchId + NOTE_CH_OFFSET, note, x, y , z);
    }

    void touchContinue(int touchId, float note, float x, float y, float z)
    {
        output_.touchContinue(touchId + NOTE_CH_OFFSET, note, x, y , z);
    }

    void touchOff(int touchId, float note, float x, float y, float z)
    {
        output_.touchOff(touchId + NOTE_CH_OFFSET);
    }

    void control(int ctrlId, float v)
    {
        output_.control(GLOBAL_CH, ctrlId, v);
    }
private:
    MecPreferences prefs_;
    MidiOutput output_;
};


void *mecapi_proc(void * arg)
{
    static int exitCode = 0;

    LOG_0( "mecapi_proc start");
    MecPreferences prefs(arg);

    if (!prefs.exists("mec") || !prefs.exists("mec-app")) {
        exitCode = 1; // fail
        pthread_exit(&exitCode);
    }

    MecPreferences app_prefs(prefs.getSubTree("mec-app"));
    MecPreferences api_prefs(prefs.getSubTree("mec"));

    MecPreferences outprefs(app_prefs.getSubTree("outputs"));

    std::unique_ptr<MecApi> mecApi;
    mecApi.reset(new MecApi());

    if (outprefs.exists("midi")) {
        MecPreferences cbprefs(outprefs.getSubTree("midi"));
        MecMidiCallback *pCb = new MecMidiCallback(cbprefs);
        if (pCb->isValid()) {
            mecApi->subscribe(pCb);
        } else {
            delete pCb;
        }
    }
    if (outprefs.exists("osc")) {
        MecPreferences cbprefs(outprefs.getSubTree("osc"));
        MecOSCCallback *pCb = new MecOSCCallback(cbprefs);
        if (pCb->isValid()) {
            mecApi->subscribe(pCb);
        } else {
            delete pCb;
        }
    }
    if (outprefs.exists("console")) {
        MecPreferences cbprefs(outprefs.getSubTree("console"));
        MecConsoleCallback *pCb = new MecConsoleCallback(cbprefs);
        if (pCb->isValid()) {
            mecApi->subscribe(pCb);
        } else {
            delete pCb;
        }
    }

    mecApi->init();

    pthread_mutex_lock(&waitMtx);
    while (keepRunning)
    {
        mecApi->process();
        struct timespec ts;
        getWaitTime(ts, 1000);
        pthread_cond_timedwait(&waitCond, &waitMtx, &ts);
    }
    pthread_mutex_unlock(&waitMtx);

    // delete the api, so that it can clean up
    LOG_0( "mecapi_proc stopping");
    mecApi.reset();
    sleep(1);
    LOG_0( "mecapi_proc stopped");

    exitCode = 0; // success
    pthread_exit(nullptr);
}

