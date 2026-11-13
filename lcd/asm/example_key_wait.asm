; Demonstrates the KEY_WAIT register, which has a VARIABLE bus delay.
;
; Uses blocking (instead of polling in example_keyboard.asm) to register key presses

; TeenyAT Constants
.const PORT_A_DIR       0x8000
.const PORT_B_DIR       0x8001
.const PORT_A           0x8002
.const PORT_B           0x8003
.const RAND             0x8010
.const RAND_BITS        0x8011
.const WALL_TIME        0x8094
.const WALL_TIME_RESET  0x8095

; LCD Peripherals
.const LIVESCREEN 0x9000
.const UPDATESCREEN 0xA000
.const X1 0xD000
.const Y1 0xD001
.const X2 0xD002
.const Y2 0xD003
.const STROKE 0xD010
.const FILL 0xD011
.const DRAWFILL 0xD012
.const DRAWSTROKE 0xD013
.const UPDATE 0xE000
.const RECT 0xE010
.const LINE 0xE011
.const POINT 0xE012
.const MOUSEX 0xFFFC
.const MOUSEY 0xFFFD
.const MOUSEB 0xFFFB
.const TERM 0xFFFF
.const KEY 0xFFFE
.const KEY_REPEAT 0xFFF9
.const KEY_WAIT 0xFFFA

.const COLORAMT 50

; rA = brush X
; rB = brush Y
; rC = brush color
; rD = key pressed
; rE = scratch (time waited)

    set rE, 50000
    str [KEY_REPEAT], rE    ; held keys repeat every 50000 cycles (~50 ms at 1 MHz)

!main
    set rA, 32
    set rB, 32
    set rC, 0x0F00
    cal !draw_brush

!main_loop
    lod rD, [KEY_WAIT]          ; CPU stalls here until a key is pressed

    cmp rD, 'W'
    je !UP
    cmp rD, 'S'
    je !DOWN
    cmp rD, 'A'
    je !LEFT
    cmp rD, 'D'
    je !RIGHT

!end_movement
    add rC, COLORAMT            ; shift the color for every pixel drawn
    cal !draw_brush
    jmp !main_loop

;-----------------------------
; Draw a pixel at (rA, rB) in color rC and show it
!draw_brush
    str [X1], rA
    str [Y1], rB
    str [STROKE], rC
    str [POINT], rZ
    str [UPDATE], rZ
    ret

;-----------------------------
!UP
    cmp rB, 0
    jle !end_movement
    dec rB
    jmp !end_movement

;-----------------------------
!DOWN
    cmp rB, 63              ; the highest possible y value
    jge !end_movement
    inc rB
    jmp !end_movement

;-----------------------------
!LEFT
    cmp rA, 0
    jle !end_movement
    dec rA
    jmp !end_movement

;-----------------------------
!RIGHT
    cmp rA, 63              ; the highest possible x value
    jge !end_movement
    inc rA
    jmp !end_movement
