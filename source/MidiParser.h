#pragma once
#include <stdint.h>

struct MidiMessage {
    uint8_t status = 0;
    uint8_t size = 0;
    uint8_t data[16] = {};
};

class MidiSink {
public:
    virtual ~MidiSink() = default;
    virtual void receive(const MidiMessage& message) = 0;
};

class MidiParser {
public:
    explicit MidiParser(MidiSink& sink) : sink(sink) {}
    void reset();
    void feed(uint8_t byte);
private:
    MidiSink& sink;
    MidiMessage pending;
    uint8_t running = 0;
    uint8_t expected = 0;
    bool sysex = false;
    bool overflow = false;
};
