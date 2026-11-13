#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "tigr.h"
#include "screen.h"
#include "teenyat.h"
#include "util.h"

/*
 *  These are the start of the address ranges or the addresses themselves: 
 *  See .... document for more detail 
 * 
 *  LIVESCREENxxxx: 
 *  - 0x9000 -- 0x9FFF each address is 1 pixel in a 64x64 lcd grid
 *  - read/write
 *  - any writes to this address automatically renders to the lcd
 *  
 *  UPDATESCREENxxxx:
 *  - 0xA000 -- 0xAFFF each address is 1 pixel in a 64x64 lcd grid
 *  - read/write
 *  - any reads/writes to the update screen work with an off screen buffer
 *  
 *  X1:
 *  - 0xD000
 *  - read/write
 * 
 *  Y1:
 *  - 0xD001
 *  - read/write
 *  
 *  X2:
 *  - 0xD002
 *  - read/write
 * 
 *  Y2:
 *  - 0xD003
 *  - read/write
 *  
 *  STROKE:
 *  - 0xD010
 *  - read/write
 *  - on write sets the stroke color used by other functions
 *  - on read sends back the current stroke color
 * 
 *  FILL:
 *  - 0xD011
 *  - read/write
 *  - on write sets the current fill color used by other functions
 *  - on read sends back the current fill color
 *  
 *  DRAWFILL:
 *  - 0xD012
 *  - read/write
 *  - used by fillable draw functions such as RECT
 * 
 *  DRAWSTROKE:
 *  - 0xD013
 *  - read/write
 *  - used by fuctions such as LINE
 * 
 *  RAND:
 *  - 0xD700
 *  - read only
 *  - returns an ensured 16 bit random number
 *  
 *  UPDATE:
 *  - 0xE000
 *  - write only
 *  - fills the screen with whatever is in the update buffer
 *  
 *  RECT:
 *  - 0xE010
 *  - write only
 *  - draws a rectangle to the update buffer using X1,Y1 & X2,Y2 as corners
 *  - will draw a stroke and fill if the corresponding boolean is nonzero
 *  
 *  LINE:
 *  - 0xE011
 *  - write only
 *  - draws a line to update buffer using X1,Y2 & X2,Y2 
 *  - color is based on STROKE
 *  
 *  POINT:
 *  - 0xE012
 *  - read/write 
 *  - on read returns color found in update buffer at coordinate X1,Y1 
 *  - on write draws point to the update buffer using X1,Y1
 *  - color is based on STROKE
 *  
 *  MOUSEX:
 *  - 0xFFFC
 *  - read only
 *  - returns the current mouse X coordinate
 * 
 *  MOUSEY:
 *  - 0xFFFD
 *  - read only
 *  - returns the current mouse Y coordinate
 * 
 *  MOUSEB:
 *  - 0xFFFB
 *  - read only
 *  - returns the current mouse button 
 *  - 0 means no button
 *  - 1 means left button
 *  - 2 means other button
 *  
 *  TERM:
 *  - 0xFFFF
 *  - write only
 *  - prints value passed into it to the terminal
 *  
 *  KEY:
 *  - 0xFFFE
 *  - read only
 *  - returns current key down
 *
 *  KEY_WAIT:
 *  - 0xFFFA
 *  - read only
 *  - returns the next key press, waiting for one if none is ready
 *  - variable delay: the LOD stays in progress (no instructions run) until a
 *    key is pressed
 * 
 *  KEY_REPEAT:
 *  - 0xFFF9
 *  - read/write
 *  - how many cycles a held key waits before KEY_WAIT returns it again
 *  - 0 (default) turns repeat off: KEY_WAIT returns each key once per press
 */

#define LIVESCREEN_START 0x9000 
#define LIVESCREEN_END 0x9FFF 
#define UPDATESCREEN_START 0xA000
#define UPDATESCREEN_END 0xAFFF
#define X1 0xD000
#define Y1 0xD001
#define X2 0xD002
#define Y2 0xD003
#define STROKE 0xD010
#define FILL 0xD011
#define DRAWFILL 0xD012
#define DRAWSTROKE 0xD013
#define RAND 0xD700
#define UPDATE 0xE000
#define RECT 0xE010
#define LINE 0xE011
#define POINT 0xE012 
#define MOUSEX 0xFFFC
#define MOUSEY 0xFFFD
#define MOUSEB 0xFFFB 
#define TERM 0xFFFF
#define KEY 0xFFFE
#define KEY_WAIT 0xFFFA
#define KEY_REPEAT 0xFFF9

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
void bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay);
void continue_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay);
static void key_wait(teenyat *t, tny_word *data);

static uint64_t key_repeat_cycles = 0;

int main(int argc, char *argv[])
{   
    if(argc < 2) {
        std::cout << "Please provide a binary file" << std::endl;
        return 1;
    }

    /* database fabulous lavender for our color model */
    initScreen(0xA81); 

    std::string fileName = argv[1];
    teenyat t;
    FILE *bin_file = fopen(fileName.c_str(), "rb");
    if(bin_file != NULL) {
        tny_init_from_file(&t, bin_file);
        tny_set_bus_read_cb(&t, bus_read);
        tny_set_bus_write_cb(&t, bus_write);
        tny_set_continue_bus_read_cb(&t, continue_bus_read);
        fclose(bin_file);
    }else {
        std::cout << "Failed to init bin file (invalid path?)" << std::endl;
        return 0;
    }

    while(!tigrClosed(window) && !tigrKeyDown(window, TK_ESCAPE)) {
        if(current_frame > (gridLength * gridLength * gridLength)){
            tigrUpdate(window);
            current_frame = 0;
        }
        if(key_pressed(window)) {
            tny_external_interrupt(&t, TNY_XINT0);
        }
        tny_clock(&t);
        current_frame++;
    }

    tigrFree(window);
    return EXIT_SUCCESS;
}

void bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay)
{
    /* Handles pixel screen reads */
    if(addr >= UPDATESCREEN_START && addr <= UPDATESCREEN_END) {
        int index = map(addr, UPDATESCREEN_START, UPDATESCREEN_END, 0, (gridLength * gridLength) - 1);
        data->u = update_screen[index];
        return;
    }

    if(addr >= LIVESCREEN_START && addr <= LIVESCREEN_END) {
        int index = map(addr, LIVESCREEN_START, LIVESCREEN_END, 0, (gridLength * gridLength) - 1);
        data->u = live_screen[index];
        return;
    }

    switch(addr) {
    case X1:
        data->u = lcd_x1;
        break;
    case Y1:
        data->u = lcd_y1;
        break;
    case X2:
        data->u = lcd_x2;
        break;
    case Y2:
        data->u = lcd_y2;
        break;
    case STROKE:
        data->u = currStrokeColor;
        break;
    case FILL:
        data->u = currFillColor;
        break;
    case DRAWFILL:
        data->u = drawFill;
        break;
    case DRAWSTROKE:
        data->u = drawStroke;
        break;
    case RAND:
        data->u = rand16();
        break;
    case MOUSEX:
        tigrMouse(window, &mouseX, &mouseY, &mouseButton);
        data->u = (uint16_t)(mouseX / pixelSize);
        break;
    case MOUSEY:
        tigrMouse(window, &mouseX, &mouseY, &mouseButton);
        data->u = (uint16_t)(mouseY / pixelSize);
        break;
    case MOUSEB:
        tigrMouse(window, &mouseX, &mouseY, &mouseButton);
        if(mouseButton != 0 && mouseButton != 1) mouseButton = 2;
        data->u = mouseButton;
        break;
    case POINT:
        data->u = pointColor();
        break;
    case KEY:
        process_keyboard_input(window);
        data->u = buffer_pop(keyboard_input_buffer,KEYBOARD_INPUT_BUFFER_SIZE);
        /* Reading a key requires a HUGE delay */
        *delay = 10000;
        break;
    case KEY_REPEAT:
        data->u = key_repeat_cycles;
        break;
    case KEY_WAIT:
        key_wait(t, data);
        break;
    default:
        break;
    }
    return;
}

void continue_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t * /*delay*/)
{
    if(addr == KEY_WAIT) {
        if(t->cycle_cnt % 10000 == 0) {
            tigrUpdate(window);
        }
        key_wait(t, data);
    }
}

/* Return the next key for KEY_WAIT, or keep the LOD waiting. */
static void key_wait(teenyat *t, tny_word *data)
{
    static int last_key = 0;
    static uint64_t last_cycle = 0;

    if(keyboard_input_buffer[0] != 0) {
        last_key = buffer_pop(keyboard_input_buffer, KEYBOARD_INPUT_BUFFER_SIZE);
        last_cycle = t->cycle_cnt;
        data->u = last_key;
        return;
    }

    if(last_key != 0 && !tigrKeyHeld(window, last_key)) {
        last_key = 0;
    }

    if(key_repeat_cycles != 0 && last_key != 0 &&
       t->cycle_cnt - last_cycle >= key_repeat_cycles) {
        last_cycle = t->cycle_cnt;
        data->u = last_key;
        return;
    }

    tny_bus_pending(t);
}

void bus_write(teenyat * /*t*/, tny_uword addr, tny_word data, uint16_t * /*delay*/)
{
    
    /* Handles pixel screen writes */
    if(addr >= UPDATESCREEN_START && addr <= UPDATESCREEN_END) {
        int index = map(addr, UPDATESCREEN_START, UPDATESCREEN_END, 0, (gridLength * gridLength) - 1);
        update_screen[index] = data.u;
        return;
    }

    if(addr >= LIVESCREEN_START && addr <= LIVESCREEN_END) {
        int index = map(addr, LIVESCREEN_START, LIVESCREEN_END, 0, (gridLength * gridLength) - 1);
        live_screen[index] = data.u;
        render();
        return;
    }

    switch(addr) {
    case X1:
        setVal(data.u, &lcd_x1);
        break;
    case Y1:
        setVal(data.u, &lcd_y1);
        break;
    case X2:
        setVal(data.u, &lcd_x2);
        break;
    case Y2:
        setVal(data.u, &lcd_y2);
        break;
    case STROKE:
        setStroke(data.u);
        break;
    case FILL:
        setFill(data.u);
        break;
    case DRAWFILL:
        setVal(data.u, &drawFill);
        break;
    case DRAWSTROKE:
        setVal(data.u, &drawStroke);
        break;
    case UPDATE:
        update();
        break;
    case RECT:
        rect();
        break;
    case LINE:
        line();
        break;
    case POINT:
        point();
        break;
    case TERM:
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(4) << data.u;
        std::cout << std::dec << std::setfill(' ') << std::setw(5);
        std::cout << "    unsigned: " << data.u;
        std::cout << "    signed: " << data.s;
        std::cout << "    char: ";
        if(data.u < 256) {
            std::cout << (char)(data.u);
        }
        else {
            std::cout << "<out of range>";
        }
        std::cout << std::endl;
        break;
    case KEY_REPEAT:
        key_repeat_cycles = data.u;
        break;
    default:
        break;
    }
    return;
}
