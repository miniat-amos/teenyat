# TeenyAT Edison Experiment Board

![edison demos](../docs/img/edison_demo.gif)

Welcome to the **TeenyAT** Edison Experiment Board! This system provides a custom pcb to develop interesting
demos using TeenyAT assembly. Featuring a 20x4 terminal lcd and numerous peripheral devices we give you full control
of the system to do whatever you or [Leroy](../docs/leroy.md) desires!

Some of the features include:

- a 20x4 terminal lcd featuring ASCII and 104 unique sprites
- 8 seven segment displays with multiple different rendering modes
- 8 reactive push buttons
- 4 toggleable dip swithes
- 2 sets of 16 fadeable leds
- 2 buzzers with sine and triangle wave forms
- 2 linear faders
- 1 four directional d-pad

The Edison's buttons and peripherals are linked to the TeenyATs 2 different 16-bit ports allowing for highly interactive programs


## Memory Mapped I/O

| **Name**            | **Address Range**      | **Access**     | **Description**                                                                 |
|---------------------|------------------------|----------------|---------------------------------------------------------------------------------|
| LCD_CURSOR           | 0xA000                 | Read/Write     | Puts a character at the cursor then advances the cursor <br> bit15 = rotate 180 ; bit14 = flip horizontal ; bit13 = flip vertical  |
| LCD_CLEAR_SCREEN     | 0A001                  | Write          | Clears the terminal (preserves cursor position)                                                  |
| LCD_MOVE_LEFT        | 0xA002                 | Read/Write     | Moves cursor X positions to the left does not wrap                                               |
| LCD_MOVE_RIGHT       | 0xA003                 | Read/Write     | Moves cursor X positions to the right does not wrap                                |
| LCD_MOVE_UP          | 0xA004                 | Read/Write     | Moves cursor X positions up does not wrap                                          |
| LCD_MOVE_DOWN        | 0xA005                 | Read/Write     | Moves cursor X positions down does not wrap                                        |
| LCD_MOVE_LEFT_WRAP   | 0xA006                 | Read/Write     | Moves cursor X positions to the left does wrap                                     |
| LCD_MOVE_RIGHT_WRAP  | 0xA007                 | Read/Write     | Moves cursor X positions to the right does wrap                                    |
| LCD_MOVE_UP_WRAP     | 0xA008                 | Read/Write     | Moves cursor X positions up does wrap                                              |
| LCD_MOVE_DOWN_WRAP   | 0xA009                 | Read/Write     | Moves cursor X positions down does wrap                                            |
| LCD_CURSOR_X         | 0xA00A                 | Read/Write     | Sets the cursor's x to the value does wrap                                         |
| LCD_CURSOR_Y         | 0xA00B                 | Read/Write     | Sets the cursor's y to the value does wrap                                         |
| LCD_CURSOR_XY        | 0xA00C                 | Read/Write     | Sets the cursor's xy to the value does wrap <br> byte1 = X ; byte0 = Y                           |
| BUZZER_LEFT          | 0xA010                 | Write          | Plays sine/triange wave on left buzzer at given frequency  <br> bit15 = triangle wave            |
| BUZZER_RIGHT         | 0xA011                 | Write          | Plays sine/triange wave on right buzzer at given frequency <br> bit15 = triange wave |
| BUZZER_DURATION      | 0xA012                 | Read/Write     | Note length in milliseconds used by BUZZER_LEFT_NOTE / BUZZER_RIGHT_NOTE |
| BUZZER_LEFT_NOTE     | 0xA013                 | Write          | Plays a note (same format as BUZZER_LEFT) for BUZZER_DURATION ms, then stops <br> **Variable delay:** the STR waits until the note finishes ; 0 = rest |
| BUZZER_RIGHT_NOTE    | 0xA014                 | Write          | Same as BUZZER_LEFT_NOTE for the right buzzer |
| FADER_LEFT           | 0xA020                 | Read           | Returns value between 0-100 based on left fader's position                         |
| FADER_RIGHT          | 0xA021                 | Read           | Returns value between 0-100 based on right fader's position                        |
