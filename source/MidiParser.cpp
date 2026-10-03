#include "MidiParser.h"

void MidiParser::reset()
{
    pending = {}; running = 0; expected = 0; sysex = false; overflow = false;
}

void MidiParser::feed(uint8_t byte)
{
    // Realtime bytes can occur between ANY two bytes, including inside SysEx.
    if(byte >= 0xf8) {
        if(byte == 0xff) { reset(); MidiMessage m; m.status = byte; sink.receive(m); }
        return;
    }
    if(sysex) {
        if(byte == 0xf7) {
            if(!overflow) sink.receive(pending);
            reset(); return;
        }
        if(!(byte & 0x80)) {
            if(pending.size < sizeof(pending.data)) pending.data[pending.size++] = byte;
            else overflow = true;
            return;
        }
        // An unexpected non-realtime status aborts SysEx and starts a new message.
        reset();
    }
    if(byte & 0x80) {
        pending = {}; expected = 0;
        if(byte < 0xf0) {
            running = byte;
            pending.status = byte;
            expected = (byte & 0xe0) == 0xc0 ? 1 : 2;
        } else {
            running = 0;
            pending.status = byte;
            if(byte == 0xf0) { sysex = true; overflow = false; }
            else if(byte == 0xf1 || byte == 0xf3) expected = 1;
            else if(byte == 0xf2) expected = 2;
        }
        return;
    }
    if(!expected) {
        if(!running) return;
        pending = {}; pending.status = running;
        expected = (running & 0xe0) == 0xc0 ? 1 : 2;
    }
    pending.data[pending.size++] = byte;
    if(pending.size == expected) {
        sink.receive(pending);
        pending = {}; expected = 0;
    }
}
