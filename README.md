# ikb_np
Ideal Key Bundle used for data Noise Protection

ToDo:
+ Message transmission via shared memory.
+ Decoder.
+ Show decoding error message in the buffer.
+ Noise for encoded data.
> Cutting codes length.
- Code refactoring.

Cosmetic improvements:
- Monospace fonts in TextView and labels.
- About program window.

Bugs:
- When send message with 97+ symbols, program stucks.
  Depending on IKB properties. Looks like incorrect buffer size calculation.
+ RX buffer set from paralel thread is failing.
  Need to move it to GTK thread.
+ Issue with incorrect data transmission after space symbol.
+ "Hello, Boris!" seg.fault.
  Lack of memory for TX & RX buffers.

Help:
* Examine shared memory:
  xxd /dev/shm/physical_channel