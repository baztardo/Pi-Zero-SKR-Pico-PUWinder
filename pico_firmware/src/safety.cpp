#include "safety.h"

void check_endstops(MoveQueue* mq) {
    if (!gpio_get(Y_MIN_ENDSTOP_PIN)) {
        printf("EMERGENCY: Y-MIN triggered!\n");
        mq->set_enable(AXIS_TRAVERSE, false);
        mq->clear_queue(AXIS_TRAVERSE);
    }
    if (!gpio_get(Y_MAX_ENDSTOP_PIN)) {
        printf("EMERGENCY: Y-MAX triggered!\n");
        mq->set_enable(AXIS_TRAVERSE, false);
        mq->clear_queue(AXIS_TRAVERSE);
    }
}