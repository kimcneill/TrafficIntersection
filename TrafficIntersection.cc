#include <omnetpp.h>

using namespace omnetpp;

// VehicleSource: creates vehicles at random intervals
class VehicleSource : public cSimpleModule
{
  private:
    cMessage *generateEvent = nullptr;

  protected:
    virtual void initialize() override {
        generateEvent = new cMessage("generateVehicle");
        scheduleAt(simTime() + par("interarrivalTime").doubleValue(), generateEvent);
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == generateEvent) {
            cMessage *vehicle = new cMessage("vehicle");
            vehicle->setTimestamp(simTime());   // used later for delay calculation
            send(vehicle, "out");

            scheduleAt(simTime() + par("interarrivalTime").doubleValue(), generateEvent);
        }
    }

    virtual void finish() override {
        cancelAndDelete(generateEvent);
    }
};

Define_Module(VehicleSource);


// TrafficQueue: forwards vehicles to intersection
class TrafficQueue : public cSimpleModule
{
  protected:
    virtual void handleMessage(cMessage *msg) override {
        send(msg, "out");
    }
};

Define_Module(TrafficQueue);


// TrafficController: switches between NS and EW green
// kind = 1 means green, kind = 0 means yellow/red
class TrafficController : public cSimpleModule
{
  private:
    cMessage *phaseTimer = nullptr;
    int phase = 0;

  protected:
    virtual void initialize() override {
        phaseTimer = new cMessage("phaseTimer");
        scheduleAt(simTime(), phaseTimer);
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == phaseTimer) {
            double greenTime = par("greenTime").doubleValue();
            double yellowTime = par("yellowTime").doubleValue();

            if (phase == 0) {
                // NS green, EW red
                send(new cMessage("NS_GREEN", 1), "nsSignal");
                send(new cMessage("EW_RED", 0), "ewSignal");
                phase = 1;
                scheduleAt(simTime() + greenTime, phaseTimer);
            }
            else if (phase == 1) {
                // yellow/all-red transition
                send(new cMessage("NS_YELLOW", 0), "nsSignal");
                send(new cMessage("EW_RED", 0), "ewSignal");
                phase = 2;
                scheduleAt(simTime() + yellowTime, phaseTimer);
            }
            else if (phase == 2) {
                // EW green, NS red
                send(new cMessage("NS_RED", 0), "nsSignal");
                send(new cMessage("EW_GREEN", 1), "ewSignal");
                phase = 3;
                scheduleAt(simTime() + greenTime, phaseTimer);
            }
            else {
                // yellow/all-red transition
                send(new cMessage("NS_RED", 0), "nsSignal");
                send(new cMessage("EW_YELLOW", 0), "ewSignal");
                phase = 0;
                scheduleAt(simTime() + yellowTime, phaseTimer);
            }
        }
    }

    virtual void finish() override {
        cancelAndDelete(phaseTimer);
    }
};

Define_Module(TrafficController);


// Intersection: stores NS/EW vehicles and releases them during green phases

class Intersection : public cSimpleModule
{
  private:
    cQueue nsQueue;
    cQueue ewQueue;

    bool nsGreen = false;
    bool ewGreen = false;

    cMessage *serviceTimer = nullptr;
    double serviceTime = 2.0; // seconds per vehicle crossing

    int vehiclesServed = 0;
    double totalDelay = 0.0;
    int maxNSQueue = 0;
    int maxEWQueue = 0;

  protected:
    virtual void initialize() override {
        serviceTimer = new cMessage("serviceTimer");
    }

    void scheduleServiceIfNeeded() {
        if (!serviceTimer->isScheduled()) {
            if ((nsGreen && !nsQueue.isEmpty()) || (ewGreen && !ewQueue.isEmpty())) {
                scheduleAt(simTime(), serviceTimer);
            }
        }
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == serviceTimer) {
            cMessage *vehicle = nullptr;

            if (nsGreen && !nsQueue.isEmpty()) {
                vehicle = check_and_cast<cMessage *>(nsQueue.pop());
            }
            else if (ewGreen && !ewQueue.isEmpty()) {
                vehicle = check_and_cast<cMessage *>(ewQueue.pop());
            }

            if (vehicle != nullptr) {
                double delay = (simTime() - vehicle->getTimestamp()).dbl();
                totalDelay += delay;
                vehiclesServed++;

                send(vehicle, "out");
            }

            if ((nsGreen && !nsQueue.isEmpty()) || (ewGreen && !ewQueue.isEmpty())) {
                scheduleAt(simTime() + serviceTime, serviceTimer);
            }
        }
        else if (msg->arrivedOn("nsIn")) {
            nsQueue.insert(msg);

            if (nsQueue.getLength() > maxNSQueue) {
                maxNSQueue = nsQueue.getLength();
            }

            scheduleServiceIfNeeded();
        }
        else if (msg->arrivedOn("ewIn")) {
            ewQueue.insert(msg);

            if (ewQueue.getLength() > maxEWQueue) {
                maxEWQueue = ewQueue.getLength();
            }

            scheduleServiceIfNeeded();
        }
        else if (msg->arrivedOn("nsSignal")) {
            nsGreen = msg->getKind() == 1;
            delete msg;
            scheduleServiceIfNeeded();
        }
        else if (msg->arrivedOn("ewSignal")) {
            ewGreen = msg->getKind() == 1;
            delete msg;
            scheduleServiceIfNeeded();
        }
    }

    virtual void finish() override {
        recordScalar("vehiclesServed", vehiclesServed);

        if (vehiclesServed > 0) {
            recordScalar("averageDelay", totalDelay / vehiclesServed);
        }

        recordScalar("maxNSQueueLength", maxNSQueue);
        recordScalar("maxEWQueueLength", maxEWQueue);

        cancelAndDelete(serviceTimer);

        while (!nsQueue.isEmpty()) {
            delete check_and_cast<cMessage *>(nsQueue.pop());
        }

        while (!ewQueue.isEmpty()) {
            delete check_and_cast<cMessage *>(ewQueue.pop());
        }
    }
};

Define_Module(Intersection);


// VehicleSink: counts completed vehicles
class VehicleSink : public cSimpleModule
{
  private:
    int count = 0;

  protected:
    virtual void handleMessage(cMessage *msg) override {
        count++;
        delete msg;
    }

    virtual void finish() override {
        recordScalar("vehiclesExitedSystem", count);
    }
};

Define_Module(VehicleSink);
