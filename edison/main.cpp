#include <iostream>
#include <chrono>
#include <thread>

#include "teenyat.h"
#include "tigr.h"
#include "edison_board.h"
#include "lcd_screen.h"
#include "leds.h"
#include "utils.h"
#include "audio.h"
#include "segment.h"
#include "edison_sprite_locations.h"

/*
 *  These are the start of the address ranges or the addresses themselves: 
 *  See .... document for more detail 
 * 
 *  LCD_CURSOR: 
 *  - 0xA000
 *  - read/write
 *  - on write puts char at current cursor position and advances cursor forward 1
 *  - on read returns char at current cursor position
 * 
 *  LCD_CLEAR_SCREEN: 
 *      - 0xA001
 *      - write 
 *      - on write clears screen (preserves cursor position)
 * 
 *  LCD_MOVE_LEFT: 
 *      - 0xA002
 *      - read/write
 *      - on write moves the cursor X positions to the left does not wrap
 *      - on read returns character 1 positon to the left does not wrap
 * 
 *  LCD_MOVE_RIGHT: 
 *      - 0xA003
 *      - read/write
 *      - on write moves the cursor X positions to the right does not wrap 
 *      - on read returns character 1 positon to the right does not wrap
 * 
 *  LCD_MOVE_UP: 
 *      - 0xA004
 *      - read/write
 *      - on write moves the cursor X positions up does not wrap 
 *      - on read returns character 1 positon up does not wrap
 * 
 *  LCD_MOVE_DOWN: 
 *      - 0xA005
 *      - read/write
 *      - on write moves the cursor X positions down does not wrap 
 *      - on read returns character 1 positon down does not wrap
 * 
 *  LCD_MOVE_LEFT_WRAP: 
 *      - 0xA006
 *      - read/write
 *      - on write moves the cursor X positions to the left does wrap
 *      - on read returns character 1 positon to the left does wrap
 * 
 *  LCD_MOVE_RIGHT_WRAP: 
 *      - 0xA007
 *      - read/write
 *      - on write moves the cursor X positions to the right does wrap 
 *      - on read returns character 1 positon to the right does wrap
 * 
 *  LCD_MOVE_UP_WRAP: 
 *      - 0xA008
 *      - read/write
 *      - on write moves the cursor X positions up does wrap 
 *      - on read returns character 1 positon up does wrap
 * 
 *  LCD_MOVE_DOWN_WRAP: 
 *      - 0xA009
 *      - read/write
 *      - on write moves the cursor X positions down does wrap 
 *      - on read returns character 1 positon down does wrap
 * 
 *  LCD_CURSOR_X: 
 *      - 0xA00A
 *      - read/write
 *      - on write sets cursors x to value value wraps around
 *      - on read returns current cursor x position
 *  
 *  LCD_CURSOR_Y: 
 *      - 0xA00B
 *      - read/write
 *      - on write sets cursors y to value (value wraps around)
 *      - on read returns current cursor y position
 * 
 *  LCD_CURSOR_XY: 
 *      - 0xA00C
 *      - read/write
 *      - on write sets cursors xy to value (values wraps around) 
 *      - on read returns current cursor xy position
 *      - byte1 = x byte0 = y
 * 
 *  FADER_LEFT:
 *      - 0xA020
 *      - read only 
 *      - returns value between 0-100 based on left fader's position (bottom = 0)
 * 
 *  FADER_RIGHT:
 *      - 0xA021
 *      - read only 
 *      - returns value between 0-100 based on right fader's position (bottom = 0)
 *  
 *  BUZZER_LEFT:
 *      - 0xA010
 *      - write only 
 *      - plays sine/triangle wave at given frequency using bit 15 as control
 *      - send zero to turn off (regardless of control bit being set)
 *      - PLEASE NOTE buzzers are peripherals pressing pause will not stop them from playing
 *
 * 
 *  BUZZER_RIGHT:
 *      - 0xA011
 *      - write only 
 *      - plays sine/triangle wave at given frequency using bit 15 as control
 *      - send zero to turn off (regardless of control bit being set)
 *      - PLEASE NOTE buzzers are peripherals pressing pause will not stop them from playing
 *
 *  BUZZER_DURATION:
 *      - 0xA012
 *      - read/write
 *      - length in milliseconds used by BUZZER_LEFT_NOTE / BUZZER_RIGHT_NOTE
 *
 *  BUZZER_LEFT_NOTE:
 *      - 0xA013
 *      - write only
 *      - same value format as BUZZER_LEFT (frequency, bit 15 = triangle wave)
 *      - plays the note for BUZZER_DURATION ms, then stops the buzzer
 *      - variable delay: the STR stays in progress (no instructions run) until
 *        the note has finished playing.  A frequency of 0 is a rest.
 *      - with a duration of 0 it behaves like BUZZER_LEFT (no wait, no stop)
 *
 *  BUZZER_RIGHT_NOTE:
 *      - 0xA014
 *      - write only
 *      - same as BUZZER_LEFT_NOTE, but for the right buzzer
*/

#define LCD_CURSOR 0xA000 
#define LCD_CLEAR_SCREEN 0xA001
#define LCD_MOVE_LEFT 0xA002  
#define LCD_MOVE_RIGHT 0xA003
#define LCD_MOVE_UP 0xA004
#define LCD_MOVE_DOWN 0xA005
#define LCD_MOVE_LEFT_WRAP 0xA006  
#define LCD_MOVE_RIGHT_WRAP 0xA007
#define LCD_MOVE_UP_WRAP 0xA008
#define LCD_MOVE_DOWN_WRAP 0xA009
#define LCD_CURSOR_X 0xA00A
#define LCD_CURSOR_Y 0xA00B
#define LCD_CURSOR_XY 0xA00C
#define BUZZER_LEFT 0xA010
#define BUZZER_RIGHT 0xA011
#define BUZZER_DURATION 0xA012
#define BUZZER_LEFT_NOTE 0xA013
#define BUZZER_RIGHT_NOTE 0xA014
#define FADER_LEFT 0xA020
#define FADER_RIGHT 0xA021


void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
void port_change(teenyat *t, bool is_port_a, tny_word port);
void continue_bus_write(teenyat *t, tny_uword addr, uint16_t *delay);

/* State for BUZZER_*_NOTE: length of the next note and when the current one ends */
static uint16_t buzzer_duration_ms = 0;
static std::chrono::steady_clock::time_point note_end_time;

int main(int argc, char* argv[])
{  
    if(argc < 2) {
        std::cout << "Please provide a binary file" << std::endl;
        return 1;
    }

    std::string fileName = argv[1];
    teenyat t;
    FILE *bin_file = fopen(fileName.c_str(), "rb");
    if(bin_file != NULL) {
        tny_init_from_file(&t, bin_file);
        tny_set_bus_read_cb(&t, bus_read);
        tny_set_bus_write_cb(&t, bus_write);
        tny_set_port_change_cb(&t,port_change);
        tny_set_continue_bus_write_cb(&t, continue_bus_write);
        fclose(bin_file);
    }else {
        std::cout << "Failed to init bin file (invalid path?)" << std::endl;
        return 0;
    }

    int failed_audio_init = audio_device_init();
    if(failed_audio_init){
        std::cout << "Failed to initialize audio" << std::endl;
        return EXIT_FAILURE;
    }

    int failed_init = initialize_board();
    if(failed_init){
        std::cout << "Failed to initialize board" << std::endl;
        return EXIT_FAILURE;
    }

    auto last_update_time = std::chrono::steady_clock::now();
    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {
        process_mouse(&t);
        process_keyboard(&t);
        if(!CLOCK_PAUSED) tny_clock(&t);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);
        
        /* 20 Hz === 50 ms */
        if(duration.count() >= 50) {
            /* Render all components so aplhas fill out  */
            lcd_render_full_screen();
            led_array_draw(&t); 
            segment_render_display(&t);
            linear_fader_render();
            render_buzzers();
            last_update_time = now;
            tigrUpdate(window); 
        }
    }

    kill_board();
    free_audio();
    return EXIT_SUCCESS;
}

void bus_read(teenyat * /*t*/, tny_uword addr, tny_word *data, uint16_t */*delay*/)
{
    switch(addr){
        case LCD_CURSOR:
            data->u = lcd_data[lcd_cursor.u].u;
            break;
        case LCD_MOVE_LEFT:
            data->u = lcd_move_cursor_x_y(-1,false,false,true).u;
            break;   
        case LCD_MOVE_RIGHT:
            data->u = lcd_move_cursor_x_y(1,false,false,true).u;
            break;   
        case LCD_MOVE_UP:
            data->u = lcd_move_cursor_x_y(-1,false,true,true).u;
            break;   
        case LCD_MOVE_DOWN:
            data->u = lcd_move_cursor_x_y(1,false,true,true).u;
            break;   
        case LCD_MOVE_LEFT_WRAP:
            data->u = lcd_move_cursor_x_y(-1,true,false,true).u;
            break;   
        case LCD_MOVE_RIGHT_WRAP:
            data->u = lcd_move_cursor_x_y(1,true,false,true).u;
            break;   
        case LCD_MOVE_UP_WRAP:
            data->u = lcd_move_cursor_x_y(-1,true,true,true).u;
            break;   
        case LCD_MOVE_DOWN_WRAP:
            data->u = lcd_move_cursor_x_y(1,true,true,true).u;
            break;  
        case LCD_CURSOR_X: 
            data->u = lcd_cursor_x_y.bytes.byte1;
            break;
        case LCD_CURSOR_Y: 
            data->u = lcd_cursor_x_y.bytes.byte0;
            break;
        case LCD_CURSOR_XY: 
            data->u = lcd_cursor_x_y.u;
            break; 
        case FADER_LEFT:
            /* Calculate reverse percentage for left fader */
            data->u = 100 - (((float)(fader_values[0] - LOC_FADER_LEFT_TL_BR[0][1]) / (LOC_FADER_LEFT_TL_BR[1][1] - LOC_FADER_LEFT_TL_BR[0][1])) * 100);
            break;
        case FADER_RIGHT:
            /* Calculate reverse percentage for right fader */
            data->u = 100 - (((float)(fader_values[1] - LOC_FADER_RIGHT_TL_BR[0][1]) / (LOC_FADER_RIGHT_TL_BR[1][1] - LOC_FADER_RIGHT_TL_BR[0][1])) * 100);
            break;
        case BUZZER_DURATION:
            data->u = buzzer_duration_ms;
            break;
        default:
            break;
    }
}


void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t */*delay*/)
{
    switch(addr){
        case LCD_CURSOR:
            lcd_draw_character(data);
            break;
        case LCD_CLEAR_SCREEN:
            lcd_clear_screen();
            break; 
        case LCD_MOVE_LEFT:
            lcd_move_cursor_x_y(-data.u,false,false,false);
            break;   
        case LCD_MOVE_RIGHT:
            lcd_move_cursor_x_y(data.u,false,false,false);
            break;   
        case LCD_MOVE_UP:
            lcd_move_cursor_x_y(-data.u,false,true,false);
            break;   
        case LCD_MOVE_DOWN:
            lcd_move_cursor_x_y(data.u,false,true,false);
            break;   
        case LCD_MOVE_LEFT_WRAP:
            lcd_move_cursor_x_y(-data.u,true,false,false);
            break;   
        case LCD_MOVE_RIGHT_WRAP:
            lcd_move_cursor_x_y(data.u,true,false,false);
            break;   
        case LCD_MOVE_UP_WRAP:
            lcd_move_cursor_x_y(-data.u,true,true,false);
            break;   
        case LCD_MOVE_DOWN_WRAP:
            lcd_move_cursor_x_y(data.u,true,true,false);
            break; 
        case LCD_CURSOR_X: 
            lcd_set_cursor_x_y(data,false,false);
            break;
        case LCD_CURSOR_Y: 
            lcd_set_cursor_x_y(data,true,false);
            break;
        case LCD_CURSOR_XY: 
            lcd_set_cursor_x_y(data,false,true);
            break;
        case BUZZER_LEFT:
            buzzer_state[0] = data.u & 0x7FFFF;
            play_sound(data.u,0);
            break;
        case BUZZER_RIGHT:
            buzzer_state[1] = data.u & 0x7FFF;
            play_sound(data.u,1);
            break;
        case BUZZER_DURATION:
            buzzer_duration_ms = data.u;
            break;
        case BUZZER_LEFT_NOTE:
            buzzer_state[0] = data.u & 0x7FFF;
            play_sound(data.u,0);
            if(buzzer_duration_ms > 0) {
                /* Keep the STR in progress until the note has played */
                note_end_time = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(buzzer_duration_ms);
                tny_bus_pending(t);
            }
            break;
        case BUZZER_RIGHT_NOTE:
            buzzer_state[1] = data.u & 0x7FFF;
            play_sound(data.u,1);
            if(buzzer_duration_ms > 0) {
                /* Keep the STR in progress until the note has played */
                note_end_time = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(buzzer_duration_ms);
                tny_bus_pending(t);
            }
            break;
        default:
            break;
    }
}

void continue_bus_write(teenyat *t, tny_uword addr, uint16_t * /*delay*/)
{
    bool note_over = std::chrono::steady_clock::now() >= note_end_time;

    switch(addr){
        case BUZZER_LEFT_NOTE:
            if(note_over) {
                buzzer_state[0] = 0;
                play_sound(0,0);
            }else {
                tny_bus_pending(t);
            }
            break;
        case BUZZER_RIGHT_NOTE:
            if(note_over) {
                buzzer_state[1] = 0;
                play_sound(0,1);
            }else {
                tny_bus_pending(t);
            }
            break;
        default:
            break;
    }
}

void port_change(teenyat *t, bool /*is_port_a*/, tny_word /*port*/){
    led_array_draw(t);
}