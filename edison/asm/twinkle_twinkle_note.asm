; "Twinkle Twinkle Little Star" played with the BUZZER_RIGHT_NOTE register,
; which has a VARIABLE bus delay.
;
; Based on the arrangement in twinkle_twinkle.asm by John Pertell.


; TeenyAT Constants
.const PORT_A_DIR       0x8000
.const PORT_B_DIR       0x8001
.const PORT_A           0x8002
.const PORT_B           0x8003
.const RAND             0x8010
.const RAND_BITS        0x8011

; Edison Constants
.const LCD_CURSOR           0xA000
.const LCD_CLEAR_SCREEN     0xA001
.const LCD_CURSOR_XY        0xA00C
.const BUZZER_LEFT          0xA010
.const BUZZER_RIGHT         0xA011
.const BUZZER_DURATION      0xA012
.const BUZZER_LEFT_NOTE     0xA013
.const BUZZER_RIGHT_NOTE    0xA014

; Buzzer Note Constants
.const SOUND_REST       0x0000  ; silence
.const SOUND_C4         0x0105  ; C4 (261 Hz)
.const SOUND_D4         0x0125  ; D4 (293 Hz)
.const SOUND_E4         0x0149  ; E4 (329 Hz)
.const SOUND_F4         0x015D  ; F4 (349 Hz)
.const SOUND_G4         0x0188  ; G4 (392 Hz)
.const SOUND_A4         0x01B8  ; A4 (440 Hz)

; Note lengths in milliseconds (120 beats per minute)
.const QUARTER          500
.const HALF             1000

    jmp !main

!twinkle_str
.raw "Twinkle Twinkle"

;------------------------------------------
; The song: pairs of (note, length in ms).  A length of 0 ends the song.
!song
; "Twinkle, twinkle, little star"
.raw SOUND_C4 QUARTER  SOUND_C4 QUARTER  SOUND_G4 QUARTER  SOUND_G4 QUARTER
.raw SOUND_A4 QUARTER  SOUND_A4 QUARTER  SOUND_G4 HALF
; "How I wonder what you are"
.raw SOUND_F4 QUARTER  SOUND_F4 QUARTER  SOUND_E4 QUARTER  SOUND_E4 QUARTER
.raw SOUND_D4 QUARTER  SOUND_D4 QUARTER  SOUND_C4 HALF
; "Up above the world so high"
.raw SOUND_G4 QUARTER  SOUND_G4 QUARTER  SOUND_F4 QUARTER  SOUND_F4 QUARTER
.raw SOUND_E4 QUARTER  SOUND_E4 QUARTER  SOUND_D4 HALF
; "Like a diamond in the sky"
.raw SOUND_G4 QUARTER  SOUND_G4 QUARTER  SOUND_F4 QUARTER  SOUND_F4 QUARTER
.raw SOUND_E4 QUARTER  SOUND_E4 QUARTER  SOUND_D4 HALF
; "Twinkle, twinkle, little star"
.raw SOUND_C4 QUARTER  SOUND_C4 QUARTER  SOUND_G4 QUARTER  SOUND_G4 QUARTER
.raw SOUND_A4 QUARTER  SOUND_A4 QUARTER  SOUND_G4 HALF
; "How I wonder what you are"
.raw SOUND_F4 QUARTER  SOUND_F4 QUARTER  SOUND_E4 QUARTER  SOUND_E4 QUARTER
.raw SOUND_D4 QUARTER  SOUND_D4 QUARTER  SOUND_C4 HALF
; end of song
.raw SOUND_REST 0

;------------------------------------------
; rA = note frequency
; rB = string pointer (for printing)
; rC = pointer into the song table
; rD = note length in ms
!main
    set rB, !twinkle_str
    cal !print_string_rB

    set rC, !song
!song_loop
    lod rA, [rC]                    ; note
    lod rD, [rC + 1]                ; length
    cmp rD, rZ
    je !end                         ; length 0 = end of song

    str [BUZZER_DURATION], rD
    str [BUZZER_RIGHT_NOTE], rA     ; plays the note; returns when it is done

    add rC, 2
    jmp !song_loop

!end
    jmp !end

;------------------------------------------
; Print String Routine
!print_string_rB
    psh rB
    psh rA
    !print_string_rB_loop
        lod rA, [rB]
        cmp rA, rZ
        je !print_string_rB_terminated
        str [LCD_CURSOR], rA
        inc rB
        jmp !print_string_rB_loop
    !print_string_rB_terminated
        pop rA
        pop rB
    ret
