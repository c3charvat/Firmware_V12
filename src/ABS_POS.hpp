#ifndef __ABS_POS_H__
#define __ABS_POS_H__

// Varibles 


// Gloabl Varble Fucntions

int get_XPOS_LIMITS();
int get_YPOS_LIMITS();
int get_AOA_T_POS_LIMITS();
int get_AOA_B_POS_LIMITS();


//void set_XPOS_MAX(float xmax, float xmin);
//void set_YPOS_MAX(float ymax, float ymin);
//void set_AOA_T_POS_MAX(float aoatmax, float aoatmin);
//void set_AOA_B_POS_MAX(float aoabmax, float aoabmin);
void set_all(float xmax, float xmin,float ymax, float ymin, float aoatmax, float aoatmin,float aoabmax, float aoabmin);

// libary function
float ABS_POS(float input, int selection);
#endif