#ifndef EPAPER_HPP
#define EPAPER_HPP

#include <esp_heap_caps.h>
#include <epdiy.h>
#include <epd_highlevel.h>
#include <stdio.h>
#include "esp_log.h"
#include "OpenSans_Condensed-Bold-8.h"
#include "OpenSans_SemiCondensed-Bold-10.h"
#include "OpenSans_SemiCondensed-Bold-12.h"
#include "OpenSans_SemiCondensed-Medium-24.h"
#include "img_home.h"

#define WAVEFORM EPD_BUILTIN_WAVEFORM
#define DEMO_BOARD epd_board_v7
#define MAX_DAYS 31

class EPaper {
public:

   enum class TEXT_ALIGN {
        Left,
        Center,
        Right
    };

    struct Coordinates{
        int x;
        int y;
    };

    const EpdFont* font_tiny = &OpenSans_8;
    const EpdFont* font_sml = &OpenSans_10;
    const EpdFont* font_mid = &OpenSans_12;
    const EpdFont* font_header = &OpenSans_24;

    EPaper();
    ~EPaper();

    int getWidth();
    int getHeight();

    void initialize();
    void splash();
    void drawBar(int x, int y, int width, int height);

    void drawProgressBar(int bar_x, int bar_y, int percent);
    void drawText(const EpdFont* font, TEXT_ALIGN align, int text_x, int text_y, const char* string);
    void drawCalendarBase(int offset_pos, int max_date, const char* title, int t_day);
    Coordinates getCoordinatesForDay(int day);
    void drawTextInSlot(int slot, int cursor_x, int cursor_y, const char *text);
    void invalidate();

private:
    const uint8_t white = 0xFF;
    const uint8_t black = 0x0;
    EpdiyHighlevelState hl;        // High-level EPD handler
    int temp;        // Ambient temperature
    uint8_t* fb;     // Framebuffer
    Coordinates day_coords[MAX_DAYS + 1]; // Array to store coordinates for each day (1 to 31)

    void checkError(enum EpdDrawError err);
    void draw_progress_bar(int x, int y, int width, int percent, uint8_t* fb);
    
};

#endif // EPAPER_HPP
