#include "tusb.h"
#include "pico/unique_id.h"
#include <cstring>

// TinyUSB example VID: local development only; obtain an assigned VID/PID for distribution.
static const tusb_desc_device_t device = {
    sizeof(tusb_desc_device_t), TUSB_DESC_DEVICE, 0x0200,
    0, 0, 0, CFG_TUD_ENDPOINT0_SIZE,
    0xcafe, 0x4018, 0x0100, 1, 2, 3, 1
};

extern "C" uint8_t const* tud_descriptor_device_cb()
{
    return reinterpret_cast<const uint8_t*>(&device);
}

static const uint8_t configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1,2,0,TUD_CONFIG_DESC_LEN+TUD_MIDI_DESC_LEN,0,100),
    TUD_MIDI_DESCRIPTOR(0,4,0x01,0x81,64)
};

extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; return configuration;
}

extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t language)
{
    (void)language;
    static uint16_t result[33];
    static char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES*2+1];
    if(index == 0) { result[0] = (TUSB_DESC_STRING<<8)|4; result[1] = 0x0409; return result; }
    const char* text = nullptr;
    switch(index) {
    case 1: text = "Pico GM Project"; break;
    case 2: text = "Pico GM MIDI"; break;
    case 3: pico_get_unique_board_id_string(serial,sizeof(serial)); text = serial; break;
    case 4: text = "Pico GM MIDI"; break;
    default: return nullptr;
    }
    size_t count = std::strlen(text);
    if(count > 32) count = 32;
    for(size_t i = 0; i < count; ++i) result[i+1] = static_cast<uint8_t>(text[i]);
    result[0] = static_cast<uint16_t>((TUSB_DESC_STRING<<8)|(2*count+2));
    return result;
}
