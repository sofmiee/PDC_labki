#ifndef LABA_2_TASK_H
#define LABA_2_TASK_H

struct Task {
    int id;
    int duration;
    int priority;
    bool is_critical;
    bool is_subtask = false;


    bool operator<(const Task & other) const {
        if (is_critical != other.is_critical) {
            return !is_critical;
        } else {
            return priority > other.priority;
        }
    }
};


#endif //LABA_2_TASK_H