#ifndef __Serial_Com_hpp__
#define __Serial_Com_hpp__

#include <SpeedyStepper.h>

// Varibles 
extern volatile bool x1home;
extern volatile bool x2home;
extern volatile bool y1home;
extern volatile bool y2home;
extern volatile bool y3home;
extern volatile bool y4home;
extern volatile bool aoathome;
extern volatile bool aoabhome;

extern float Xpos;
extern float Ypos;
extern float AoA[];

extern int Acell_Data[];
extern int Speed_Data[];

extern int Com_selection;

extern int Micro_stepping[];

extern int DRIVER_ADDRESS;

extern void HomeAll();
extern void MOVE_FUNCTION();
extern void SET_ACELL(float x, float y, float E0, float E1);
extern void SET_SPEED(int x, int y, int E0, int E1);

bool recvWithStartEndMarkers(bool newData, char receivedChars[], byte numChars);

bool parseData();

void showParsedData();

void gui_output_function();

void serial_flush_buffer();

#endif