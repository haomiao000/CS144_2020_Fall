#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.
#include <cstdint>
template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}
#include <algorithm>
using namespace std;
#include<iostream>
//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return isn + static_cast<uint32_t>(n); }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // cout<<wrap(static_cast<uint64_t>(8223994832456613924), WrappingInt32(4097058588))<<endl;
    uint64_t tmp =
        (n.raw_value() >= isn.raw_value()) ? n.raw_value() - isn.raw_value() : (1ul<<32) - (isn.raw_value() - n.raw_value());
    uint32_t dv = checkpoint / (1ul << 32);
    uint64_t cnt1 = dv * (1ul << 32) + tmp;
    uint64_t cnt2 = (dv - 1) * (1ul << 32) + tmp;
    uint64_t cnt3 = (dv + 1) * (1ul << 32) + tmp;
    uint64_t c1 = (checkpoint > cnt1) ? (checkpoint - cnt1) : (cnt1 - checkpoint);
    uint64_t c2 = (checkpoint > cnt2) ? (checkpoint - cnt2) : (cnt2 - checkpoint);
    uint64_t c3 = (checkpoint > cnt3) ? (checkpoint - cnt3) : (cnt3 - checkpoint);
    if (c1 <= c2 && c1 <= c3) {
        return cnt1;
    }
    else if (c2 <= c1 && c2 <= c3){
        return cnt2;
    }
    else{
        return cnt3;
    }
}
