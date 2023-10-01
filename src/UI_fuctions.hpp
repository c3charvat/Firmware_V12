#ifndef __UI_Functions_hpp__
#define __UI_Functions_hpp__
#include <U8g2lib.h>

extern uint8_t current_selection;

extern float Xpos;
extern float Ypos;
extern float AoA[];

extern int Acell_Data[];
extern int Speed_Data[];

extern int Com_selection;
extern int Sub_selection;

extern int Micro_stepping[];

void MAIN_MENU();
void SERIAL_UI(void);
void Draw_userinput(const char *title, const char *pre, float *DisplayValue, float lo, float hi, const char *post, float increments[]);
int check_button_event();
void Draw_button(U8G2 u8g2, uint8_t x, uint8_t y, uint8_t width, String str, bool clicked);
void Draw_dialog(U8G2 u8g2, uint8_t x, uint8_t y, uint8_t width, uint8_t height, String title, String pre, String value, String increment, String post, String btn, bool clicked);

#endif