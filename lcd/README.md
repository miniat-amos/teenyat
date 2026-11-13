# TeenyAT Color LCD

![lcd demos](../docs/img/lcd_demo.gif)

Welcome to the **TeenyAT** color LCD! This system provides a creative workspace to develop
interesting and unique graphical assembly programs. The LCD includes a 64 x 64 pixel array that can be written to in order to control the output of the screen. The LCD exposes two distinct screen areas for addressing pixels:

- a live screen that updates instantly
- an off-screen buffer for precalculating pixel values

The Color LCD also supports both mouse and keyboad support as well as the ability to print out register values to the terminal

## Memory Mapped I/O

| **Name**            | **Address Range**      | **Access**     | **Description**                                                                 |
|---------------------|------------------------|----------------|---------------------------------------------------------------------------------|
| LIVESCREEN       | 0x9000 – 0x9FFF         | Read/Write     | Each address is 1 pixel in 64×64 LCD; writes render directly to the display    |
| UPDATESCREEN     | 0xA000 – 0xAFFF         | Read/Write     | Off-screen buffer                                 |
| X1                   | 0xD000                 | Read/Write     | X-coordinate 1 for drawing                                                      |
| Y1                   | 0xD001                 | Read/Write     | Y-coordinate 1 for drawing                                                      |
| X2                   | 0xD002                 | Read/Write     | X-coordinate 2 for drawing                                                      |
| Y2                   | 0xD003                 | Read/Write     | Y-coordinate 2 for drawing                                                      |
| STROKE               | 0xD010                 | Read/Write     | Set/get stroke color                                                            |
| FILL                 | 0xD011                 | Read/Write     | Set/get fill color                                                              |
| DRAWFILL             | 0xD012                 | Read/Write     | Enable/disable fill for shapes                                                  |
| DRAWSTROKE           | 0xD013                 | Read/Write     | Enable/disable stroke for shapes                                                |
| UPDATE               | 0xE000                 | Write          | Pushes the update buffer to the live screen                                     |
| RECT                 | 0xE010                 | Write          | Draw rectangle using X1,Y1 and X2,Y2; uses FILL and STROKE settings           |
| LINE                 | 0xE011                 | Write          | Draw line from X1,Y1 to X2,Y2 with STROKE color                                 |
| POINT                | 0xE012                 | Read/Write     | Read: get color at X1,Y1 <br> Write: draw point at X1,Y1 with STROKE color          |
| MOUSEB               | 0xFFFB                 | Read           | Mouse button: 0 = none, 1 = left, 2 = other                                     |
| MOUSEX               | 0xFFFC                 | Read           | Current mouse X position                                                        |
| MOUSEY               | 0xFFFD                 | Read           | Current mouse Y position                                                        |
| KEY                  | 0xFFFE                 | Read           | Returns current key pressed                                                     |
| KEY_WAIT             | 0xFFFA                 | Read           | Returns the next key press, waiting for one if none is ready; held keys repeat if KEY_REPEAT is set <br> **Variable delay:** no instructions run until a key is pressed |
| KEY_REPEAT           | 0xFFF9                 | Read/Write     | Cycles a held key waits before KEY_WAIT returns it again ; 0 = no repeat (default) <br> e.g. 50000 = ~50 ms at 1 MHz |
| TERM                 | 0xFFFF                 | Write          | Output value to terminal                                                        |
