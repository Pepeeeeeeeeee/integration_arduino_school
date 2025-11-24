#include "MeasurementBuffer.h"
#include <esp_heap_caps.h>

MeasurementBuffer::MeasurementBuffer()
    : _buffer(nullptr), _capacity(0), _head(0), _tail(0), _count(0) {}

MeasurementBuffer::~MeasurementBuffer() {
    heap_caps_free(_buffer);
}

void MeasurementBuffer::begin(size_t capacity) {
    _capacity = capacity;

    if (_capacity > 0) {
        _buffer = (Measurement*)heap_caps_malloc(_capacity * sizeof(Measurement), MALLOC_CAP_DEFAULT);
        if (_buffer == nullptr) {
            Serial.println("[Buffer] Memory allocation failed!");
            _capacity = 0;
        } else {
            Serial.printf("[Buffer] Initialized with capacity for %u measurements.\n", _capacity);
        }
    } else {
        Serial.println("[Buffer] Capacity set to 0. Buffer not initialized.");
    }
}

bool MeasurementBuffer::push(const Measurement& measurement) {
    if (isFull()) {
        return false;
    }
    _buffer[_head] = measurement;
    _head = (_head + 1) % _capacity;
    _count++;
    return true;
}

bool MeasurementBuffer::pop(Measurement& measurement) {
    if (isEmpty()) {
        return false;
    }
    measurement = _buffer[_tail];
    _tail = (_tail + 1) % _capacity;
    _count--;
    return true;
}

bool MeasurementBuffer::peek(Measurement& measurement) const {
    if (isEmpty()) {
        return false;
    }
    measurement = _buffer[_tail];
    return true;
}

bool MeasurementBuffer::isFull() const {
    return _count == _capacity;
}

bool MeasurementBuffer::isEmpty() const {
    return _count == 0;
}

size_t MeasurementBuffer::getCount() const {
    return _count;
}

size_t MeasurementBuffer::getCapacity() const {
    return _capacity;
}