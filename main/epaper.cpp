#include "epaper.hpp"

static const char *TAG = "[E-Paper]";

#define calrendar_rect_width 114
#define calrendar_rect_height 110

const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

EPaper::EPaper() : temp(0), fb(nullptr) {}

EPaper::~EPaper() {
    // Cleanup if needed
}

void EPaper::checkError(enum EpdDrawError err) {
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "draw error: %X", err);
    }
}

void EPaper::initialize() {
    // Initialize the E-Paper display
    epd_init(&DEMO_BOARD, &ED097TC2, EPD_LUT_64K);
    epd_set_vcom(1560);

    // Initialize the high-level EPD handler
    hl = epd_hl_init(WAVEFORM);

    // Set orientation to portrait mode
    epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);

    ESP_LOGI(TAG,
        "Dimensions after rotation, width: %d height: %d\n\n",
        epd_rotated_display_width(),
        epd_rotated_display_height()
    );

    heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);

    // Get framebuffer
    fb = epd_hl_get_framebuffer(&hl);
}

void EPaper::splash(){
    epd_poweron();
    epd_clear();
    temp = epd_ambient_temperature();
    epd_poweroff();
    
    epd_hl_set_all_white(&hl);

    EpdRect home_area = {
        .x = epd_rotated_display_width() / 2 - img_home_width / 2, 
        .y = epd_rotated_display_height() / 2 - img_home_height,
        .width = img_home_width,
        .height = img_home_height,
    };

    epd_draw_rotated_image(home_area, img_home_data, fb);
    epd_poweron();
    checkError(epd_hl_update_screen(&hl, MODE_GL16, temp));
    epd_poweroff();
}

void EPaper::drawBar(int x, int y, int width, int height){
    EpdRect bar = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };

    epd_fill_rect(bar, black, fb);
}

void EPaper::draw_progress_bar(int x, int y, int width, int percent, uint8_t* fb) {
 

    EpdRect border = {
        .x = x,
        .y = y,
        .width = width,
        .height = 20,
    };
    epd_fill_rect(border, white, fb);
    epd_draw_rect(border, black, fb);

    EpdRect bar = {
        .x = x + 5,
        .y = y + 5,
        .width = (width - 10) * percent / 100,
        .height = 10,
    };

    epd_fill_rect(bar, black, fb);

    checkError(epd_hl_update_area(&hl, MODE_DU, epd_ambient_temperature(), border));
}

void EPaper::drawProgressBar(int bar_x, int bar_y, int percent){
    epd_poweron();
    draw_progress_bar(bar_x, bar_y, 400, percent, fb);
    epd_poweroff();
}

void EPaper::drawText(const EpdFont* font, TEXT_ALIGN align, int text_x, int text_y, const char* string) {
    
    EpdFontProperties font_props = epd_font_properties_default();
    switch (align) {
        case TEXT_ALIGN::Left:
            font_props.flags = EPD_DRAW_ALIGN_LEFT;
            break;
        case TEXT_ALIGN::Center:
            font_props.flags = EPD_DRAW_ALIGN_CENTER;
            break;
        case TEXT_ALIGN::Right:
            font_props.flags = EPD_DRAW_ALIGN_RIGHT;
            break;
    }
    epd_write_string(font, string, &text_x, &text_y, fb, &font_props);

    // Update screen
    epd_poweron();
    checkError(epd_hl_update_screen(&hl, MODE_GL16, temp));
    epd_poweroff();
}

void EPaper::drawCalendarBase(int offset_pos, int max_date, const char* title, int t_day){
    epd_poweron();
    epd_clear();
    temp = epd_ambient_temperature();
    epd_poweroff();

    epd_hl_set_all_white(&hl);

    drawText(font_header, TEXT_ALIGN::Left, 22, 60, title);

    EpdFontProperties font_props = epd_font_properties_default();
    font_props.flags = EPD_DRAW_ALIGN_CENTER;

    EpdFontProperties font_props_2 = epd_font_properties_default();
    font_props_2.flags = EPD_DRAW_ALIGN_RIGHT;

    EpdFontProperties font_props_3 = epd_font_properties_default();
    font_props_3.flags = EPD_DRAW_ALIGN_RIGHT;
    font_props_3.fg_color = white;
    
    
    int current_day = 1;  // Start from the first day of the month

    int text_x = 70;
    int text_y = 100;
    for (int x = -1; x < 6; x++) {
        for (int y = 0; y < 7; y++) {

            if(x == -1){ // Draw Calendar Headers
                text_x = 65 + (y * calrendar_rect_width);
                text_y = 108;
                epd_write_string(font_mid, days[y], &text_x, &text_y, fb, &font_props);

            }else{
                if (offset_pos > 0) {  // Adjust starting position based on the first day of the week
                    offset_pos--;
                    continue;
                }

                if (current_day > max_date) {  // Stop drawing once we pass the last day of the month
                    break;
                }

                int cursor_x = 10 + (y * calrendar_rect_width);
                int cursor_y = 120 + (x * calrendar_rect_height);

                day_coords[current_day].x = cursor_x;
                day_coords[current_day].y = cursor_y;


                // Define the rectangle for the current day
                EpdRect border = {
                    .x = cursor_x,
                    .y = cursor_y,
                    .width = calrendar_rect_width,
                    .height = calrendar_rect_height,
                };

                  EpdRect sub_border = {
                    .x = cursor_x + 70,
                    .y = cursor_y ,
                    .width = 44,
                    .height = 40,
                };


          

                epd_poweron();
                epd_draw_rect(border, black, fb);   // Draw the border of the rectangle
                
                 if(current_day == t_day){
                    epd_fill_rect(sub_border, black, fb); 
                 }else{
                    epd_draw_rect(sub_border, black, fb);   // Draw the border of the rectangle
                 }

                char sdate[2];
                sprintf(sdate, "%d", current_day);
                int date_x = cursor_x + 108;
                int date_y = cursor_y + 26;

                if(current_day == t_day){
                                    epd_write_string(font_mid, sdate, &date_x, &date_y, fb, &font_props_3);
                 }else{
                                    epd_write_string(font_mid, sdate, &date_x, &date_y, fb, &font_props_2);
                 }


                    

                checkError(epd_hl_update_area(&hl, MODE_DU, temp, border));
                epd_poweroff();

                current_day++;  // Move to the next day
            }
          
        }
    }
}

void EPaper::drawTextInSlot(int slot, int cursor_x, int cursor_y, const char *text) {

    EpdFontProperties font_props = epd_font_properties_default();
    font_props.flags = EPD_DRAW_ALIGN_LEFT;
    
    int text_x = cursor_x + 4;
    int text_y = cursor_y + (24 * slot);
    epd_write_string(font_mid, text, &text_x, &text_y, fb, &font_props);
}

EPaper::Coordinates EPaper::getCoordinatesForDay(int day) {
    if (day < 1 || day > MAX_DAYS) {
        return { -1, -1 }; // Invalid coordinates
    }
    return day_coords[day];
}


int EPaper::getWidth() {
    return epd_rotated_display_width();
}

int EPaper::getHeight(){
    return epd_rotated_display_height();
}

void EPaper::invalidate(){
    // Update screen
    epd_poweron();
    checkError(epd_hl_update_screen(&hl, MODE_GL16, temp));
    epd_poweroff();
}