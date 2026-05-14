//
// TrafficIntersection.ned
// Four-way traffic light timing simulation model

simple VehicleSource
{
    parameters:
        volatile double interarrivalTime @unit(s);
    gates:
        output out;
}

simple TrafficQueue
{
    gates:
        input in[];
        output out;
}

simple TrafficController
{
    parameters:
        double greenTime @unit(s);
        double yellowTime @unit(s);
        double redTime @unit(s);
    gates:
        output nsSignal;
        output ewSignal;
}

simple Intersection
{
    gates:
        input nsIn;
        input ewIn;
        input nsSignal;
        input ewSignal;
        output out;
}

simple VehicleSink
{
    gates:
        input in;
}

network TrafficIntersection
{
    submodules:
        northSource: VehicleSource {
            parameters:
                @display("p=100,100;i=block/source");
        }

        southSource: VehicleSource {
            parameters:
                @display("p=100,220;i=block/source");
        }

        eastSource: VehicleSource {
            parameters:
                @display("p=100,340;i=block/source");
        }

        westSource: VehicleSource {
            parameters:
                @display("p=100,460;i=block/source");
        }

        nsQueue: TrafficQueue {
            parameters:
                @display("p=320,160;i=block/queue");
        }

        ewQueue: TrafficQueue {
            parameters:
                @display("p=320,400;i=block/queue");
        }

        controller: TrafficController {
            parameters:
                @display("p=540,280;i=block/cogwheel");
        }

        intersection: Intersection {
            parameters:
                @display("p=750,280;i=block/switch");
        }

        sink: VehicleSink {
            parameters:
                @display("p=950,280;i=block/sink");
        }

    connections:
        northSource.out --> nsQueue.in++;
        southSource.out --> nsQueue.in++;

        eastSource.out --> ewQueue.in++;
        westSource.out --> ewQueue.in++;

        nsQueue.out --> intersection.nsIn;
        ewQueue.out --> intersection.ewIn;

        controller.nsSignal --> intersection.nsSignal;
        controller.ewSignal --> intersection.ewSignal;

        intersection.out --> sink.in;
}