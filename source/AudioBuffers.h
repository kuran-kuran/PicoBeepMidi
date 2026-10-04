#pragma once
#include <stdint.h>

// Dma starts one finite, unchained transfer. Only submit() may start it again.
// A late producer leaves the last PWM value in place; DMA never walks past
// a buffer and the producer never writes into the buffer being played.
template<class Dma, unsigned Frames = 128>
class AudioBuffers {
public:
    static constexpr unsigned frames = Frames;
    explicit AudioBuffers(Dma& transfer) : dma(transfer) {}

    void start(uint32_t silence)
    {
        for(auto& block : blocks) for(auto& sample : block) sample = silence;
        filling = 1;
        dma.start(blocks[0],Frames);
    }
    uint32_t* writable() { return blocks[filling]; }
    bool submit()
    {
        const bool underrun = !dma.busy();
        while(dma.busy()) dma.idle();
        // The old transfer is finished, and the new block is fully rendered.
        dma.start(blocks[filling],Frames);
        filling ^= 1;
        return underrun;
    }
private:
    Dma& dma;
    alignas(4) uint32_t blocks[2][Frames];
    unsigned filling = 1;
};
