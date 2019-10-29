# ITM Console (or soon to be terminal)

UART console on STM32F4 on monochromatic ITM-400160K1-01 screen (400x160 pixels)


## Hardware features:
- UART interrupt, DMA (not yet xD)
- Screen interrupt, DMA (not yet xD)
- async screen erasing? (disabled for now cause cursor was being erased :)

Implement me:
- Scroll wheel for scrollback buffer
- USB keyboard (to make it a fully functional terminal)


## Software features:
- rxtv-basic compatible (?; write own termcap?)
  + text rendering
  + basic cursor movement ([A[B[C[D [H [f)
  + alternate screen ([?1047h [?1047l)
  + screen/line erasing
- custom escape code for image rendering ([width;height^base64-data)
- sleep when not receiving much data?
- bell (inverse screen for a moment)


## Tools
- image converter script
- bitmap font converter (todo)


TODO:
- adjust contrast depending on how many chars there are on the screen
  foreach byte in screen: lit += byte -> contrast output = lit / bytes
  contrast output would be connected to contrast circuitry which adjusts it somehow
- utf8 reading
- respond with cursor position when asked
- respond with other stuff when asked (like terminal info)
- don't write \n twice if in the last column
- fix cursor erasing/displaying
