#pragma once
#include <stdint.h>

class ModeButton {
public:
    enum class Action { none, toggle_mode, reset };
    Action update(bool down, uint32_t now_us)
    {
        if(!initialized || down != candidate) {
            initialized = true; candidate = down; since = now_us;
            return Action::none;
        }
        if(static_cast<uint32_t>(now_us-since) < 30000) return Action::none;
        if(!down) armed = true;
        if(stable != down) {
            stable = down;
            if(down) {
                pressed_at = since; active = armed; fired = false;
            } else {
                if(active && !fired) {
                    active = false;
                    return static_cast<uint32_t>(since-pressed_at) >= 1000000 ?
                        Action::reset : Action::toggle_mode;
                }
                active = false;
            }
        }
        if(down && active && !fired && static_cast<uint32_t>(now_us-pressed_at) >= 1000000) {
            fired = true;
            return Action::reset;
        }
        return Action::none;
    }
private:
    bool initialized = false, candidate = false, stable = false, armed = false;
    bool active = false, fired = false;
    uint32_t since = 0, pressed_at = 0;
};
