#include "global.h"
#include "ultra64/viint.h"

OSViMode osViModeMpalLan1 = {
    OS_VI_MPAL_LAN1, // type
    {
        // comRegs
        0x311E,               // ctrl
        WIDTH(320),           // width
        BURST(57, 30, 5, 70), // burst
        VSYNC(525),           // vSync
        HSYNC(3089, 4),       // hSync
        LEAP(3097, 3098),     // leap
        HSTART(108, 748),     // hStart
        SCALE(2, 0),          // xScale
        VCURRENT(0),          // vCurrent
    },
    { // fldRegs
      {
          // [0]
          ORIGIN(640),        // origin
          SCALE(1, 0),        // yScale
          START(37, 511),     // vStart
          BURST(4, 2, 14, 0), // vBurst
          VINTR(2),           // vIntr
      },
      {
          // [1]
          ORIGIN(640),        // origin
          SCALE(1, 0),        // yScale
          START(37, 511),     // vStart
          BURST(4, 2, 14, 0), // vBurst
          VINTR(2),           // vIntr
      } },
};
