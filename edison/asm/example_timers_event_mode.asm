;---------------------------------------------------------
; Simple demonstration of the use of timers in EVENT mode.
; The program sets up an interrupt handler for Timer A that
; that will go off every time a key is pressed/released (not held)
;
;       TIMER CONTROL STATUS REGISTER ENCODING
;                0b0_00_0_000000000000
;
;           bit0 -> bit11  : Clock Prescaler (how many cycles until we increment the count?)
;           bit12          : Clock Enable    (should we run the timer)
;           bit13 -> bit14 : Clock Mode      (0->Count, 1->Level, 2->Event)
;           bit15          : Clock Reset     (sets the clocks csr, cmp and cnt to zero)
;
; In the teenyAT Edison Experiment Board Timer A is hooked up to track the key
; presses in the experiment board

; TeenyAT Constants
.const PORT_A_DIR                0x8000
.const PORT_B_DIR                0x8001
.const PORT_A                    0x8002
.const PORT_B                    0x8003

.const RAND                      0x8010
.const RAND_BITS                 0x8011

.const TIMER_A_CSR               0x8080
.const TIMER_A_CMP               0x8081
.const TIMER_A_CNT               0x8082
.const TIMER_B_CSR               0x8088
.const TIMER_B_CMP               0x8089
.const TIMER_B_CNT               0x808A

.const INTERRUPT_VECTOR_TABLE    0x8E00
.const INTERRUPT_ENABLE_REGISTER 0x8E10

.const CONTROL_STATUS_REGISTER   0x8EFF

; Edison Constants
.const LCD_CURSOR           0xA000
.const LCD_CLEAR_SCREEN     0xA001
.const LCD_MOVE_LEFT        0xA002
.const LCD_MOVE_RIGHT       0xA003
.const LCD_MOVE_UP          0xA004
.const LCD_MOVE_DOWN        0xA005
.const LCD_MOVE_LEFT_WRAP   0xA006
.const LCD_MOVE_RIGHT_WRAP  0xA007
.const LCD_MOVE_UP_WRAP     0xA008
.const LCD_MOVE_DOWN_WRAP   0xA009
.const LCD_CURSOR_X         0xA00A
.const LCD_CURSOR_Y         0xA00B
.const LCD_CURSOR_XY        0xA00C
.const BUZZER_LEFT          0xA010
.const BUZZER_RIGHT         0xA011
.const FADER_LEFT           0xA020
.const FADER_RIGHT          0xA021

; Set up interrupt callbacks for Timers A
    set rA, !timer_A_callback
    set rB, 6
    str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable internal interrupts 6 ( Timers A )
    set rA, rZ
    bts rA, 6
    str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
    set rA, rZ
    bts rA, 0
    str [ CONTROL_STATUS_REGISTER ], rA

;---------- Timer Setup ----------------
; By setting our comparison to 1 we can have the timer go off
; every time it receives an event trigger

; Timer A will go off once for every initial key press
    set rA, 1
    str [ TIMER_A_CMP ], rA
    set rA, rZ
    bts rA, 14               ; setting bit 14 puts our timer into EVENT mode
    bts rA, 12               ; enable our timer (bit12)
    str [ TIMER_A_CSR ], rA

;---------- Main Loop --------------------
; Register A holds the count of key presses/releases
    set rA, rZ
    str [ PORT_B_DIR ], rZ      ; all outputs

    str [ LCD_CURSOR_XY ], rZ
    set rC, !info1
    cal !print_string_rC
    set rC, !info2
    cal !print_string_rC

!main
    ; All updates are handled by interrupts
    jmp !main

;---------- Interrupt Callbacks ----------
!timer_A_callback
    inc rA
    str [ PORT_B ], rA
    str [ TIMER_A_CNT ], rZ     ; reset our timer
    rti

;--------- String Printing Function ------------
; Prints characters in a string pointed to by register C
!print_string_rC
    psh rA
    psh rC
!print_string_rC_loop
    lod rA, [rC]
    cmp rA, rZ
    je !print_string_rC_return
    str [ LCD_CURSOR ], rA
    inc rC
    jmp !print_string_rC_loop
!print_string_rC_return
    pop rC
    pop rA
    ret

;--------- Data ----------------
!info1
.raw "Counts Key Presses"

!info2
.raw '\n' '\r' "and Key Releases!"
