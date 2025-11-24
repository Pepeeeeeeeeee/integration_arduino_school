#ifndef MEASUREMENT_BUFFER_H
#define MEASUREMENT_BUFFER_H

#include "APIClient.h"
#include <Arduino.h>

class MeasurementBuffer {
public:
    MeasurementBuffer();
    ~MeasurementBuffer();

    void begin(size_t capacity);
    bool push(const Measurement& measurement);
    bool pop(Measurement& measurement);
    bool peek(Measurement& measurement) const;

    bool isFull() const;
    bool isEmpty() const;

    size_t getCount() const;
    size_t getCapacity() const;

private:
    Measurement* _buffer;
    size_t _capacity;
    size_t _head;
    size_t _tail;
    size_t _count;
};

#endif // MEASUREMENT_BUFFER_H
