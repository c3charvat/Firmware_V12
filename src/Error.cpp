#include <Arduino.h>
#include <U8g2lib.h>


// LCD class im not sure how to bring this class in correctly so i am just definigh it everhwere it is needed.
static U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/PE13, /* data=*/PE15, /* CS=*/PE14, /* reset=*/PE10);


// Error Menu options 
const char *Error_String = // Error strings
      " Acknowledge \n"
      " Main Menu \n"
      " Software Restart ";


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Error Messages ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool Angle_Error(int a){ // pass in an intager corresponding to the axis that has an error 
  uint8_t Error_selection;

  const char *Axis[4] = {"AoA Top\n", "AoA Bottom\n", "X Axis\n", "Y Axis\n"};
  Error_selection = u8g2.userInterfaceMessage("Error:", Axis[a-1], "input is over the max allowed\n", Error_String);  // To Do:  make this more comprhensive
  // if Com_selection = 1 the selction is serial
  //    Go to a satic screen with the read outs of each Stepper and "LCD" mode button that retuns to main menu
  // if Com_selectio == 2 selection is LCD
  // Return back to main menu

  if ( Error_selection == 1) {
    // Ok go back to where the function was called from This option works because where this error is called
    //serial_flush_buffer();
    return 0;
  }
  if ( Error_selection == 2) {
    // Head back to Main meanu
    //serial_flush_buffer();
    //MAIN_MENU();
    return 0;
  }
  return 0; //
}
void Somthing_Error(void){
  uint8_t Error_selection;
  Error_selection = u8g2.userInterfaceMessage("Error:", " Somthing Went Wrong!:\n", "\n", Error_String);  // make this more comprhensive
  // if Com_selection = 1 the selction is serial
  //    Go to a satic screen with the read outs of each Stepper and "LCD" mode button that retuns to main menu
  // if Com_selectio == 2 selection is LCD
  // Return back to main menu

  if ( Error_selection == 1) {
    // acknowledge
    //serial_flush_buffer();
    return;
    
  }
  if ( Error_selection == 2) {
    // Head back to Main meanu
    //MAIN_MENU();
    //serial_flush_buffer();
  }
  if ( Error_selection ==3){
    // software resert -> from mbed os
    NVIC_SystemReset();
  }
}
void Parsing_Error(void){
  uint8_t Error_selection;
  Error_selection = u8g2.userInterfaceMessage("Somthing Went Wrong:", "Code not Reconized:\n", "\n","ok");  // make this more comprhensive
  // if Com_selection = 1 the selction is serial
  //    Go to a satic screen with the read outs of each Stepper and "LCD" mode button that retuns to main menu
  // if Com_selectio == 2 selection is LCD
  // Return back to main menu

  if ( Error_selection == 1) {
    // Go back nothing happed return to where ever the system was
    return;
  }
}