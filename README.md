# ikb_np
Ideal Key Bundle used for data Noise Protection

ToDo:
+ Message transmission via shared memory.
> Decoder.
- Noise for encoded data.
- Cutting codes length.
- Code refactoring.

Cosmetic improvements:
- Monospace fonts in TextView and labels.
- About program window.

Bugs:
- When send message with 97+ symbols, program stucks.
  Depending on IKB properties. Looks like incorrect buffer size calculation.

Help:
- Examine shared memory:
  xxd /dev/shm/physical_channel