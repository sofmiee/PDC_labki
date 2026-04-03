#ifndef LABA_2_DATAPACKET_H
#define LABA_2_DATAPACKET_H

#include <chrono>

struct DataPacket {
    int id;
    int priority;
    bool is_critical;
    std::chrono::milliseconds processing_time;
    int station_id;

    DataPacket(int _id, int _priority, bool _critical,
               std::chrono::milliseconds _time, int _station_id)
        : id(_id), priority(_priority), is_critical(_critical),
          processing_time(_time), station_id(_station_id) {}

    bool operator<(const DataPacket& other) const {
        if (is_critical != other.is_critical) {
            return !is_critical;
        }
        if (priority != other.priority) {
            return priority > other.priority;
        }
        return false;
    }
};

#endif //LABA_2_DATAPACKET_H