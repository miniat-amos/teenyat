#include <iostream>
#include "teenyat.h"
#include "tigr.h"
#include "utils.h"
#include "edison_board.h"
#include "lcd_screen.h"
#include "leds.h"
#include "segment.h"
#include "edison_sprite_locations.h"
#include "audio.h"
#include "bin_assets.h"

Tigr* window;
Tigr* background_img;
Tigr* lcd_font_img;
Tigr* leds_img;
Tigr* push_buttons_img;
Tigr* dip_button_img;
Tigr* seg_seven_img;
Tigr* fader_slot_img;
Tigr* fader_img;
Tigr* dpad_img;
Tigr* buzzer_img;

tny_word inp_keyboard;
tny_word push_button_state;
tny_word segment_dips;
tny_word old_toggle_state;
int mouse_x;
int mouse_y;
int mouse_button;
int old_mouse_button;
int set_paused = false;
bool CLOCK_PAUSED = false;
int fader_values[2] = {LOC_FADER_LEFT_TL_BR[1][1],LOC_FADER_RIGHT_TL_BR[1][1]};
int fader_update = 0;
int buzzer_state[2] = {0};

/* Loads images along with window width and height */
int initialize_board(){

    background_img = tigrLoadImageMem(bin_asset_edison_png,
                                      bin_asset_edison_png_len);
    if(!background_img) {
        printf("Failed to load asset <edison_board>\n");
        return EXIT_FAILURE;
    }

    lcd_font_img = tigrLoadImageMem(bin_asset_teenyat_font_png,
                                    bin_asset_teenyat_font_png_len);
    if(!lcd_font_img ) {
        printf("Failed to load asset <teenyat_font>\n");
        return EXIT_FAILURE;
    }

    leds_img = tigrLoadImageMem(bin_asset_leds_png,
                                bin_asset_leds_png_len);
    if(!leds_img ) {
        printf("Failed to load asset <leds>\n");
        return EXIT_FAILURE;
    }

    push_buttons_img = tigrLoadImageMem(bin_asset_buttons_png,
                                        bin_asset_buttons_png_len);
    if(!push_buttons_img ) {
        printf("Failed to load asset <buttons>\n");
        return EXIT_FAILURE;
    }

    dip_button_img = tigrLoadImageMem(bin_asset_dips_png,
                                      bin_asset_dips_png_len);
    if(!dip_button_img ) {
        printf("Failed to load asset <dips>\n");
        return EXIT_FAILURE;
    }
    
    seg_seven_img = tigrLoadImageMem(bin_asset_seven_segment_png,
                                     bin_asset_seven_segment_png_len);
    if(!seg_seven_img ) {
        printf("Failed to load asset <seven_segment>\n");
        return EXIT_FAILURE;
    }
    
    fader_slot_img = tigrLoadImageMem(bin_asset_fader_slot_png,
                                      bin_asset_fader_slot_png_len);
    if(!fader_slot_img ) {
        printf("Failed to load asset <fader_slot>\n");
        return EXIT_FAILURE;
    }
    
    fader_img = tigrLoadImageMem(bin_asset_fader_png,
                                 bin_asset_fader_png_len);
    if(!fader_img ) {
        printf("Failed to load asset <fader>\n");
        return EXIT_FAILURE;
    }

    dpad_img = tigrLoadImageMem(bin_asset_dpad_png,
                                bin_asset_dpad_png_len);
    if(!dpad_img) {
        printf("Failed to load asset <dpad>\n");
        return EXIT_FAILURE;
    }

    buzzer_img = tigrLoadImageMem(bin_asset_buzzers_png,
                                  bin_asset_buzzers_png_len);
    if(!buzzer_img) {
        printf("Failed to load asset <buzzers>\n");
        return EXIT_FAILURE;
    }

    window = tigrWindow(background_img->w, background_img->h, "Edison Board", TIGR_FIXED);
    
    segment_dips.u = 0x0100;        // make port B dip switch init to decimal mode
    reset_board();

    return EXIT_SUCCESS;
}

/* Resets and initializes variables */
void reset_board(){
    /* only draw board background on reset */
    tigrBlit(window, background_img, 0, 0, 0, 0, background_img->w, background_img->h);
    lcd_cursor.u = 0;
    lcd_cursor_x_y.u = 0;
    push_button_state.u = 0;
    lcd_clear_screen();
    led_array_draw(NULL);
    render_push_buttons(NULL);
    render_dip_switches();
    /* kill volume on reset */
    buzzer_state[0] = 0;
    buzzer_state[1] = 0;
    play_sound(0,0);
    play_sound(0,1);
    render_buzzers();
}

/* Free all memory */
void kill_board(){
    tigrFree(window);
    tigrFree(background_img);
    tigrFree(lcd_font_img);
    tigrFree(leds_img);
    tigrFree(push_buttons_img);
    tigrFree(dip_button_img);
    tigrFree(fader_slot_img);
    tigrFree(fader_img);
    tigrFree(seg_seven_img);
    tigrFree(dpad_img);
    tigrFree(buzzer_img);
}

void render_buzzers(){
    int dest_x = LOC_BUZZER_LEFT_TL[0];
    int dest_y = LOC_BUZZER_LEFT_TL[1];
    int width = (buzzer_img->w / 2);
    int height = buzzer_img->h;
    /* Mask out the control bit of the buzzers value */
    int src_x = (buzzer_state[0] & 0x7FFF) ? width : 0;

    tigrBlit(window, buzzer_img, dest_x, dest_y, src_x, 0, width, height);

    dest_x = LOC_BUZZER_RIGHT_TL[0];
    dest_y = LOC_BUZZER_RIGHT_TL[1];
    width = (buzzer_img->w / 2);
    height = buzzer_img->h;
    /* Mask out the control bit of the buzzers value */
    src_x = (buzzer_state[1] & 0x7FFF) ? width : 0;

    tigrBlit(window, buzzer_img, dest_x, dest_y, src_x, 0, width, height);

}

/* This code currently has to manipulate the dip widths as the sprite is too big*/
void render_dip_switches(){
    int dest_x = LOC_DIPS_PORTA_TL[0];
    int dest_y = LOC_DIPS_PORTA_TL[1];
    int src_x = 0;
    int src_y = 0;
    int dip_width = (dip_button_img->w / 2);
    int dip_height = dip_button_img->h;
    int index = 7;

    for(int i = 0; i < PORTA_DIPS; i++){
        src_x = dip_width * (inp_keyboard.u & (1<<index)) >> index;
        tigrBlit(window, dip_button_img, dest_x, dest_y, src_x, src_y, dip_width, dip_height);
        dest_x += dip_width;   
        index--;
    }
    
    dest_x = LOC_7SEG_DIPS_PORTB_TL[0];
    dest_y = LOC_7SEG_DIPS_PORTB_TL[1];
    src_x = 0;
    src_y = 0;
    index = 9;
    for(int y = 0; y < SEGMENT_ROWS; y++){
        // 2 because its a 2 bit number
        for(int x = 0; x < 2; x++){
            src_x = dip_width * (segment_dips.u & (1<<index)) >> index;
            tigrBlit(window, dip_button_img, dest_x, dest_y, src_x, src_y, dip_width, dip_height);
            dest_x += dip_width; 
            index--;  
        }
        index-=6; // hop over to next byte
        dest_x = LOC_7SEG_DIPS_PORTA_TL[0];
        dest_y = LOC_7SEG_DIPS_PORTA_TL[1];
    }
}

void dpad_render(){
    int width = dpad_img->w / DPAD_COLS;
    int height = dpad_img->h / DPAD_ROWS;
    /* 4 buttons on the dpad */
    for(int index = 0; index < 4; index++){
        int dest_x = LOC_DPAD_TL[index][0];
        int dest_y = LOC_DPAD_TL[index][1];
        int src_x = index * width;
        int src_y = height * (inp_keyboard.u & (1<<index)) >> index;
        tigrBlit(window, dpad_img, dest_x, dest_y, src_x, src_y, width, height);
    }
}

void render_push_buttons(tny_word *t){

    int dest_x = LOC_BUTTONS_PORTA_TL[0];
    int dest_y = LOC_BUTTONS_PORTA_TL[1];
    int src_x = 0;
    int src_y = 0;
    int button_width = push_buttons_img->w / 2;
    int button_height = push_buttons_img->h;

    tny_word port_a;
    port_a.u = 0;
    if(t) port_a.u = t->u;
    int index = 15;
    for(int i = 0; i < BUTTONS_ROWS; i++){
        for(int j = 0; j < BUTTONS_COLS; j++){
                src_x = button_width * (port_a.u & (1<<index)) >> index;
                tigrBlit(window, push_buttons_img, dest_x, dest_y, src_x, src_y, button_width, button_height);
                dest_x += button_width;
                index--;
        } 
        dest_x = LOC_BUTTONS_PORTA_TL[0];
        dest_y += button_height;
    }

    dpad_render();
}

void linear_fader_render(){
    
    /* Draw left fader-slot */
    int dest_x = LOC_FADER_LEFT_TL_BR[0][0];
    int dest_y = LOC_FADER_LEFT_TL_BR[0][1];
    tigrBlit(window, fader_slot_img, dest_x, dest_y, 0, 0, fader_slot_img->w, fader_slot_img->h);
    
    dest_y = fader_values[0];
    tigrBlit(window, fader_img, dest_x, dest_y, 0, 0, fader_img->w, fader_img->h);

    /* Draw right fader-slot */
    dest_x = LOC_FADER_RIGHT_TL_BR[0][0];
    dest_y = LOC_FADER_RIGHT_TL_BR[0][1];
    tigrBlit(window, fader_slot_img, dest_x, dest_y, 0, 0, fader_slot_img->w, fader_slot_img->h);
    

    dest_y = fader_values[1];
    tigrBlit(window, fader_img, dest_x, dest_y, 0, 0, fader_img->w, fader_img->h);
}

void linear_fader_update(){
    if(fader_update>0){
        fader_values[fader_update-1] = mouse_y;
        if(fader_values[fader_update-1] < LOC_FADER_LEFT_TL_BR[0][1]){
                    fader_values[fader_update-1] = LOC_FADER_LEFT_TL_BR[0][1];
        }else if(fader_values[fader_update-1] > LOC_FADER_LEFT_TL_BR[1][1]){
                    fader_values[fader_update-1] = LOC_FADER_LEFT_TL_BR[1][1];
        }
    }
}

void process_keyboard(teenyat* t){
    
    /* Handle pause button */
    CLOCK_PAUSED = tigrKeyHeld(window, 'P') | set_paused;

    /* Handels button presses related to port_A */
    tny_word old_keyboard = inp_keyboard;

    inp_keyboard.bits.bit11 = tigrKeyHeld(window, 'A');
    inp_keyboard.bits.bit10 = tigrKeyHeld(window, 'S');
    inp_keyboard.bits.bit9 = tigrKeyHeld(window,  'D');
    inp_keyboard.bits.bit8 = tigrKeyHeld(window,  'F');

    inp_keyboard.bits.bit15 = tigrKeyHeld(window, 'Q');
    inp_keyboard.bits.bit14 = tigrKeyHeld(window, 'W');
    inp_keyboard.bits.bit13 = tigrKeyHeld(window, 'E');
    inp_keyboard.bits.bit12 = tigrKeyHeld(window, 'R');
    
    tny_word toggle_switches;
    toggle_switches.bits.bit4 = tigrKeyDown(window, 'L');
    toggle_switches.bits.bit5 = tigrKeyDown(window, 'K');
    toggle_switches.bits.bit6 = tigrKeyDown(window, 'J');
    toggle_switches.bits.bit7 = tigrKeyDown(window, 'H');

    if(toggle_switches.u && old_toggle_state.u != toggle_switches.u) {
        inp_keyboard.u ^= toggle_switches.u;
    }
    old_toggle_state.u = toggle_switches.u;

    inp_keyboard.bits.bit0 = tigrKeyHeld(window, TK_LEFT);
    inp_keyboard.bits.bit1 = tigrKeyHeld(window, TK_UP);
    inp_keyboard.bits.bit2 = tigrKeyHeld(window, TK_RIGHT);
    inp_keyboard.bits.bit3 = tigrKeyHeld(window, TK_DOWN);
    
    /* Mask in push_button_state */
    inp_keyboard.u |= push_button_state.u;
    tny_set_ports(t,&inp_keyboard,NULL);

    /* Render displays on port change */
    if(inp_keyboard.u ^ old_keyboard.u){
        render_dip_switches();
        render_push_buttons(&inp_keyboard);
        led_array_draw(t);   
        tny_trigger_timer_event(t, TNY_TIMER_A);
        tny_set_timer_pin(t,TNY_TIMER_A,inp_keyboard.bits.bit0);
        tny_set_timer_pin(t,TNY_TIMER_B,inp_keyboard.bits.bit2);
    }

    old_keyboard.u = inp_keyboard.u;
}

void process_mouse(teenyat* t){
    tigrMouse(window, &mouse_x, &mouse_y, &mouse_button);
    if(mouse_button != 0 && old_mouse_button == 0){
        mouse_pressed(t);
    }else if(mouse_button != 0 && old_mouse_button != 0){
        mouse_down();
    }else if(mouse_button == 0 && old_mouse_button != 0){
        mouse_released();
    }
    old_mouse_button = mouse_button;
}

void mouse_pressed(teenyat* t){
    
    /* Check collision with RESET push button */
    int dest_x = LOC_BUTTONS_RESET[0];
    int dest_y = LOC_BUTTONS_RESET[1];
    int width = push_buttons_img->w / 2;
    int height = push_buttons_img->h;
    if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
        tny_reset(t);
        reset_board();
        return;
    }
    
    /* Check collisions with port A dips */
    dest_x = LOC_DIPS_PORTA_TL[0];
    dest_y = LOC_DIPS_PORTA_TL[1];
    width = (dip_button_img->w / 2);
    height = dip_button_img->h;
    int index = 7;

    for(int i = 0; i < PORTA_DIPS; i++){
        if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
            /* mask in our dip switch values here */
            inp_keyboard.u ^= (1<<index);
            render_dip_switches();
            return;
        }
        dest_x += width;   
        index--;
    }

    /* Collision with segment dip switches */
    dest_x = LOC_7SEG_DIPS_PORTB_TL[0];
    dest_y = LOC_7SEG_DIPS_PORTB_TL[1];
    index = 9;
    for(int y = 0; y < SEGMENT_ROWS; y++){
        for(int x = 0; x < 2; x++){
            if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
                /* mask in our dip switch values here */
                segment_dips.u ^= (1<<index);
                render_dip_switches();
                return;
            }
            dest_x += width;
            index--;
        }
        index -= 6; // hop over to next byte
        dest_x = LOC_7SEG_DIPS_PORTA_TL[0];
        dest_y = LOC_7SEG_DIPS_PORTA_TL[1];
    }
}

void mouse_down(){
    linear_fader_update();
    /* Check collisions with Port A push buttons */
    int dest_x = LOC_BUTTONS_PORTA_TL[0];
    int dest_y = LOC_BUTTONS_PORTA_TL[1];
    int width = push_buttons_img->w / 2;
    int height = push_buttons_img->h;
    int index = 15;
    push_button_state.u = 0;
    for(int i = 0; i < BUTTONS_ROWS; i++){
        for(int j = 0; j < BUTTONS_COLS; j++){
                if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
                    push_button_state.u |= (1<<index);
                    return;
                }
                dest_x += width;
                index--;
        } 
        dest_x = LOC_BUTTONS_PORTA_TL[0];
        dest_y += height;
    }

    /* Check collision with PAUSE push button */
    dest_x = LOC_BUTTONS_PAUSE[0];
    dest_y = LOC_BUTTONS_PAUSE[1];
    width = push_buttons_img->w / 2;
    height = push_buttons_img->h;
    if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
        set_paused = 1;
        return;
    }

    /* Check collision with left fader */
    dest_x = LOC_FADER_LEFT_TL_BR[0][0];
    dest_y = fader_values[0];
    width = fader_img->w;
    height = fader_img->h;
    if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
            fader_update = 1;
            return;
    }

    /* Check collision with right fader */
    dest_x = LOC_FADER_RIGHT_TL_BR[0][0];
    dest_y = fader_values[1];
    width = fader_img->w;
    height = fader_img->h;
    if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
           fader_update = 2;
            return;
    }

    /* Check collision with dpad */
    width = dpad_img->w / DPAD_COLS;
    height = dpad_img->h / DPAD_ROWS;
    /* 4 buttons on the dpad */
    for(int index = 0; index < 4; index++){
        int dest_x = LOC_DPAD_TL[index][0];
        int dest_y = LOC_DPAD_TL[index][1];
        if(point_rect(mouse_x,mouse_y,dest_x,dest_y,width,height)){
                push_button_state.u |= (1<<index);
                return;
        } 
    }

}

void mouse_released(){
    push_button_state.u = 0;
    set_paused = 0;
    fader_update = 0;
}
