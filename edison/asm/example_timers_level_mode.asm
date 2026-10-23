;---------------------------------------------------------
; Simple demonstration of the use of timers in LEVEL mode.
; The program sets up an interrupt handler for Timer A & Timer B
; that will go off every second provided the associated level pin
; is HIGH
;
;       TIMER CONTROL STATUS REGISTER ENCODING
;                0b0_00_0_000000000000
;
;           bit0 -> bit11  : Clock Prescaler (how many cycles until we increment the count?)
;           bit12          : Clock Enable    (should we run the timer)
;           bit13 -> bit14 : Clock Mode      (0->Count, 1->Level, 2->Event)
;           bit15          : Clock Reset     (sets the clocks csr, cmp and cnt to zero)
;
; In the teenyAT Edison Experiment Board Timer A's level pin is linked to
; the pressing of the left arrow key while Timer B's pin is linked to the
; pressing of the right arrow key

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

; Set up interrupt callbacks for Timers A and B
    set rA, !timer_A_callback
    set rB, 6
    str [ INTERRUPT_VECTOR_TABLE + rB ], rA

    inc rB
    set rA, !timer_B_callback
    str [ INTERRUPT_VECTOR_TABLE + rB ], rA

; Enable internal interrupts 6 & 7 ( Timers A and B respectively)
    set rA, rZ
    bts rA, 6
    bts rA, 7
    str [ INTERRUPT_ENABLE_REGISTER ], rA

; Enable interrupts globally
    set rA, rZ
    bts rA, 0
    str [ CONTROL_STATUS_REGISTER ], rA

;---------- Timer Setup ----------------
; Given a 1mhz clock rate the math Im using to activate our timer every second is
; (1,000,000 / 50,000) = 20
; This means we need a clock prescaler of 20 or in other words increment our timer once every 20 cycles

; Timer A will go off once every second the left arrow is pressed
    set rA, 50000
    str [ TIMER_A_CMP ], rA
    set rA, 20               ; set our cps to be 20
    bts rA, 13               ; setting bit 13 puts our timer into LEVEL mode
    bts rA, 12               ; enable our timer (bit12)
    str [ TIMER_A_CSR ], rA

; Timer B will go off every second the right arrow is pressed
    set rA, 50000
    str [ TIMER_B_CMP ], rA
    set rA, 20               ; set our cps to be 20
    bts rA, 13               ; setting bit 13 puts our timer into LEVEL mode
    bts rA, 12               ; enable our timer (bit12)
    str [ TIMER_B_CSR ], rA

;---------- Main Loop --------------------
; Register A holds the count of seconds passed in Timer A
; Register B holds the count of seconds passed in Timer B
    set rA, rZ
    set rB, rZ
    str [ PORT_A_DIR ], rZ      ; all outputs
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

!timer_B_callback
    inc rB
    str [ PORT_A ], rB
    str [ TIMER_B_CNT ], rZ     ; reset our timer
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
.raw "Timers Rock!"

!info2
.raw '\n' '\n' '\r' "Hold Arrows <- ->"
