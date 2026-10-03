#include "ModeStorage.h"
#include "ModeJournal.h"
#include "BoardControls.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

namespace {
constexpr uint32_t offset=2*1024*1024-ModeJournal::size;
const uint8_t* data() { return reinterpret_cast<const uint8_t*>(XIP_BASE+offset); }
struct Write { ModeJournal::Plan plan; alignas(4) uint8_t page[256]; };
void __no_inline_not_in_flash_func(write_flash)(void* context) {
    auto& w=*static_cast<Write*>(context);
    const uint32_t irq=save_and_disable_interrupts();
    if(w.plan.erase_sector>=0) flash_range_erase(offset+w.plan.erase_sector*4096,4096);
    flash_range_program(offset+w.plan.page*256,w.page,256);
    restore_interrupts(irq);
}
}
uint8_t mode_storage_load() { return ModeJournal::scan(data()).mode; }
bool mode_storage_save(uint8_t mode) {
    if(mode!=1 && mode!=2) return false;
    const auto state=ModeJournal::scan(data());
    if(state.mode==mode) return true;
    static Write w;
    w.plan=ModeJournal::plan(data(),state);
    ModeJournal::encode(w.page,w.plan.sequence,mode);
    if(!board_flash_operation(write_flash,&w)) return false;
    const auto saved=ModeJournal::scan(data());
    return saved.valid && saved.sequence==w.plan.sequence && saved.mode==mode;
}
