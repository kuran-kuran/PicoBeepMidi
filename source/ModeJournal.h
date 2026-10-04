#pragma once
#include <stdint.h>
#include <string.h>

// Two sectors, sixteen independently programmed pages per sector.
namespace ModeJournal {
constexpr unsigned sector_size=4096, page_size=256, size=8192;
struct State { bool valid=false; uint32_t sequence=0; uint8_t mode=1; int page=-1; };
inline uint32_t crc(const uint8_t* p, unsigned count) {
    uint32_t c=0xffffffffu;
    while(count--) { c^=*p++; for(unsigned b=0;b<8;++b) c=(c>>1)^(0xedb88320u & (0u-(c&1))); }
    return ~c;
}
inline bool erased(const uint8_t* p) {
    for(unsigned i=0;i<page_size;++i) if(p[i]!=255) return false;
    return true;
}
inline State scan(const uint8_t* data) {
    State s;
    for(int i=0;i<32;++i) {
        const uint8_t* p=data+i*page_size;
        uint32_t magic,seq,check;
        memcpy(&magic,p,4); memcpy(&seq,p+4,4); memcpy(&check,p+12,4);
        if(magic!=0x314d4750 || (p[8]!=1 && p[8]!=2) || p[9]!=1 || check!=crc(p,12)) continue;
        if(!s.valid || (seq!=s.sequence && uint32_t(seq-s.sequence)<0x80000000u))
            s={true,seq,p[8],i};
    }
    return s;
}
struct Plan { int page; int erase_sector; uint32_t sequence; };
inline Plan plan(const uint8_t* data, State s) {
    const int sector=s.valid ? s.page/16 : 0;
    const int first=s.valid ? s.page+1 : 0;
    for(int i=first;i<(sector+1)*16;++i)
        if(erased(data+i*page_size)) return {i,-1,s.sequence+1};
    const int other=1-sector;
    return {other*16,other,s.sequence+1};
}
inline void encode(uint8_t* p,uint32_t sequence,uint8_t mode) {
    memset(p,255,page_size);
    const uint32_t magic=0x314d4750;
    memcpy(p,&magic,4); memcpy(p+4,&sequence,4); p[8]=mode; p[9]=1;
    const uint32_t check=crc(p,12); memcpy(p+12,&check,4);
}
}
