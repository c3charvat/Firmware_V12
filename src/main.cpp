
#include <Arduino.h> // Include Github links here
#include <U8g2lib.h>
#include <SpeedyStepper.h>
#include <TMCStepper.h>
#include <TMCStepper_UTILITY.h>
#include <stm32yyxx_ll_gpio.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/// Jump to bootloader stuff
typedef void (*pFunction)(void); // bootloader jump function
#define DRIVER_ADDRESS 0b00
#define BOOTLOADER_FLAG_VALUE 0xDEADBEEF
#define BOOTLOADER_FLAG_OFFSET 100
#define BOOTLOADER_ADDRESS 0x1FFF0000
#define DOCTOPUS_BOARD
#define DOCTOPUS_BOARD_FROM_HSE

// Name spaces
using namespace std;
using namespace TMC2208_n; // Allows the TMC2209 to use functions out of tmc2208 required

// TMC Stepper Class
TMC2209Stepper driverX0(PC4, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
TMC2209Stepper driverX1(PE1, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
TMC2209Stepper driverY0(PD11, PA6, .11f, DRIVER_ADDRESS);  // (RX, TX,RSENSE, Driver address) Software serial X axis
TMC2209Stepper driverY1(PC6, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
TMC2209Stepper driverY2(PD3, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
TMC2209Stepper driverY3(PC7, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver Address) Software serial X axis
TMC2209Stepper driverAOAT(PF2, PA6, .11f, DRIVER_ADDRESS); // (RX, TX,RESENSE, Driver address) Software serial X axis
TMC2209Stepper driverAOAB(PE4, PA6, .11f, DRIVER_ADDRESS); // (RX, TX,RESENSE, Driver address) Software serial X axis
                                                           // TMC2209Stepper driverE3(PE1, PA6, .11f, DRIVER_ADDRESS );  // (RX, TX,RESENSE, Driver address) Software serial X axi

// Speedy Stepper           // Octopus board plug #
SpeedyStepper X_stepper;    // motor 0
SpeedyStepper Y0_stepper;   // motor 1
SpeedyStepper Y1_stepper;   // motor 2_1
SpeedyStepper Y3_stepper;   // motor 3
SpeedyStepper AOAT_stepper; // motor 4
SpeedyStepper AOAB_stepper; // motor 5
SpeedyStepper Y2_stepper;   // motor 6
SpeedyStepper X2_stepper;   // motor 7

// LCD class im not sure how to bring this class in correctly so i am just definigh it everhwere it is needed.
static U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/PE13, /* data=*/PE15, /* CS=*/PE14, /* reset=*/PE10);

#include "Pin_Setup.hpp"
#include "ABS_POS.hpp"
#include "UI_fuctions.hpp"

// Dev Settings
bool SWD_Programing_mode = true;
bool Endstop_Bypass_enable = true;
bool Verbose_mode = true;
// Jump to bootloader stuff:
extern int _estack;
uint32_t *bootloader_flag;
pFunction JumpToApplication;
uint32_t JumpAddress;

// n pre delcared functions
void setup();
void SET_SPEED();
void SET_ACELL();
void driver_setup();
void x1HomeIsr();
void x2HomeIsr();
void y1HomeIsr();
void y2HomeIsr();
void y3HomeIsr();
void y4HomeIsr();
void aoatHomeIsr();
void aoabHomeIsr();
void motionTriggerIsr();
void estopIsr();

// Global ISR Varibles
volatile bool x1home = false;
volatile bool x2home = false;
volatile bool y1home = false;
volatile bool y2home = false;
volatile bool y3home = false;
volatile bool y4home = false;
volatile bool aoathome = false;
volatile bool aoabhome = false;

// Device Settings
const int Xpos_MAX = 390; // Max X length in MM
const int Ypos_MAX = 245; // MAy Y length in MM
const int X_Lead_p = 2;   // X lead screw pitch in mm/revolution
const int Y_Lead_p = 2;   // Y lead screw pitch in mm
const int AOA_MAX = 40;
const int AOA_MIN = -20;

/// Data Structues
int Micro_stepping[5] = {64, 64, 64, 64, 64};         // mirco stepping for the drivers
float Degree_per_step[5] = {1.8, 1.8, 1.8, 1.8, 1.8}; // mirco stepping for the drivers
int Speed_Data[5] = {0, 0, 0, 0, 0};                  // Hold the Speed Data
int Acell_Data[5] = {0, 0, 0, 0, 0};                  // Hold the acelleration data
// State machine 
float AoA[2];                                         // floating point for AoA: Top,Bottom
float Xpos;                                           // X position float value
float Ypos;                                           // Y position float value

int main()
{
  // Set globals
  set_all(Xpos_MAX, 0, Ypos_MAX, 0, AOA_MAX, AOA_MIN, AOA_MAX, AOA_MIN);
  // Run bootloader code
  // This is hella dangerous messing with the stack here but hey gotta learn somehow
  bootloader_flag = (uint32_t *)(&_estack - BOOTLOADER_FLAG_OFFSET); // below top of stack
  if (*bootloader_flag == BOOTLOADER_FLAG_VALUE)                     // Kill the Hal layer and remap memory to boot addy and then call the function to jump
  {

    *bootloader_flag = 1;
    /* Jump to system memory bootloader */
    HAL_SuspendTick();
    HAL_RCC_DeInit();
    HAL_DeInit();
    JumpAddress = *(__IO uint32_t *)(BOOTLOADER_ADDRESS + 4);
    JumpToApplication = (pFunction)JumpAddress;
    //__ASM volatile ("movs r3, #0\nldr r3, [r3, #0]\nMSR msp, r3\n" : : : "r3", "sp");
    JumpToApplication();
  }
  if (*bootloader_flag = 1) // handle the case we just got out of the jump, stm32 dunio wont bring our clocks back so we do it manually here
  {
    //__memory_changed(void);       // this was used when i was testing this code, doesnt work in a program
    HAL_RCC_DeInit();             // De init the current hsi clocks
    SystemClock_Config();         // Call the clock config function to set HSE values
    HAL_Init();                   // Init the HAL layer
    __HAL_RCC_GPIOC_CLK_ENABLE(); // Restart the GPIO CLOCKS all of them and bring us back to life.
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
  }
  *bootloader_flag = 0; // So next boot won't be affecteed

  ////// Begin normal code here

  // Menu Delerations

  // bool Abs_pos_error = false;

  const char *Setting_list = // Define the Settings Sub menu options
      "Acceleration\n"
      "Speed\n"
      "Serial Com.\n" // "Trigger Mode\n"
      "Home All Axis\n"
      "Firmware Flash\n"
      "BACK";

  const char *Com_select = // Communcations method select menu
      "SERIAL\n"
      "LCD";

  // const char *Motion_select = // motion select menu
  //     "Trigger ON\n"
  //     "Trigger OFF";

  const char *Y_or_N_String = // Error strings
      " YES \n"
      " NO ";

  /// run setup
  setup();

  X_stepper.connectToPins(MOTOR0_STEP_PIN, MOTOR0_DIRECTION_PIN);
  Y0_stepper.connectToPins(MOTOR1_STEP_PIN, MOTOR1_DIRECTION_PIN);
  Y1_stepper.connectToPins(MOTOR2_STEP_PIN, MOTOR2_DIRECTION_PIN);
  Y3_stepper.connectToPins(MOTOR3_STEP_PIN, MOTOR3_DIRECTION_PIN);
  AOAT_stepper.connectToPins(MOTOR4_STEP_PIN, MOTOR4_DIRECTION_PIN);
  AOAB_stepper.connectToPins(MOTOR5_STEP_PIN, MOTOR5_DIRECTION_PIN);
  X2_stepper.connectToPins(MOTOR6_STEP_PIN, MOTOR6_DIRECTION_PIN);
  if (SWD_Programing_mode == false)
  {
    Y2_stepper.connectToPins(MOTOR7_STEP_PIN, MOTOR7_DIRECTION_PIN);
  }

  u8g2.begin(/* menu_select_pin= */ PE7, /* menu_next_pin= */ PE12, /* menu_prev_pin= */ PE9, /* menu_home_pin= */ PC15); // pc 15 was selected at random to be an un used pin
  // Leave this outside the Pin Define and in the main dir. As it also serves as a class defintion.
  // Define the System Font see https://github.com/olikraus/u8g2/wiki/u8g2reference for more information about the commands
  u8g2.setFont(u8g2_font_6x12_tr);

  // initilise External Coms
  // handle usb connection setup
  // handle wifi connection setup
  // check connection status - if there is somthing connected or trying to connect.
  // set connection status
  // if there is somthing connected Auto change to that Com.

  // jump into main application
  // infinite loop

  uint8_t current_selection = 0; // Keep track of current selection in menu
  uint8_t Sub_selection = 0;
  uint8_t Com_selection = 2; // communications selection tracker by default use the LCD
  // uint8_t Motion_selection = 2; // Default to OFF
  //  Experimental Menu Stuff
  uint8_t button_event = 0;

  for (;;)       ///// Main LOOP
  {              // run the main application
                 // statement(s)
                 // if lcd ui
    MAIN_MENU(); // issues the main menu command
    // To Do Bring in All Global Varibles to here and form them into a struct and then pass that into the functions like Serial UI and Home all/

    //
    //  if ( current_selection == 0 ) {
    //    u8g2.userInterfaceMessage(
    //      "Nothing selected.",
    //      "",
    //      "",
    //      " ok ");
    //  }
    if (current_selection == 1)
    {
      float increments[] = {100, 10, 1, .1, .01};
      // X movement
      Draw_userinput("X position:", "  ", &Xpos, 0, Xpos_MAX, "mm", increments);
      MOVE_FUNCTION();
    }
    if (current_selection == 2)
    {
      float increments[] = {100, 10, 1, .1, .01};
      // y movement
      Draw_userinput("Y position:", "  ", &Ypos, 0, Ypos_MAX, "mm", increments);
      MOVE_FUNCTION();
    }
    if (current_selection == 3)
    {
      float increments[] = {1, .1, .01};
      // AOA Top movment
      Draw_userinput("AOA Top:", "  ", &AoA[0], -10, 30, "Degrees", increments);
      MOVE_FUNCTION();
      // 200 possible steps per revolution and 1/16 miro stepping meaning a pssiblity of 3,200 possible postions 360*/1.8 degrees/step

      // Serial.print(AoA);
      // Serial.print(H_value); // DEBUG Serial Print out
      // Serial.print(value);-
    }
    if (current_selection == 4)
    {
      // AOA Bottom Movement
      float increments[] = {1, .1, .01};
      Draw_userinput("AOA Bottom:", "  ", &AoA[1], -10, 30, "Degrees", increments);
      MOVE_FUNCTION();
    }

    if (current_selection == 5)
    {
      Sub_selection = u8g2.userInterfaceSelectionList( // Bings up the Main Menu
          "Settings",
          Sub_selection,
          Setting_list);
      //    if ( Sub_selection == 0 ) {
      //      u8g2.userInterfaceMessage(
      //        "Nothing selected.",
      //        "",
      //        "",
      //        " ok ");
      //    }
      if (Sub_selection == 1)
      {
        // Acceleration Settings Code
        u8g2.userInterfaceInputValue("Acceleration:", "", Acceleration, 0, 20, 2, "*25 Rev/s^2");
        SET_ACELL(*Acceleration * 25, *Acceleration * 25, *Acceleration, *Acceleration);
      }
      if (Sub_selection == 2)
      {
        // Speed Settings
        u8g2.userInterfaceInputValue("Speed:", "", Speed, 0, 20, 2, "*25 Rev/s");
        SET_SPEED(*Speed * 25, *Speed * 25, *Speed * 5, *Speed * 5);
      }
      if (Sub_selection == 3)
      {
        // Motion Select function
        Com_selection = 1; // switch to serial Comunications
        SERIAL_UI();       // call the serial UI
      }
      // if (Sub_selection == 4)
      // {
      //   // Trigger mode
      //   Motion_selection = u8g2.userInterfaceSelectionList( // Bings up the trigger Menu
      //       "Trigger Select",
      //       Motion_selection,
      //       Motion_select);
      // }
      if (Sub_selection == 4)
      {
        // Home All Axis
        HomeAll();
      }
      if (Sub_selection == 5)
      {
        uint8_t FW_selection = 2; // default to hovering over the No option.
        // Head back to Main meanu
        FW_selection = u8g2.userInterfaceMessage("FIRMWARE FLASH:", "Are you sure you want", "to flash firmware?", Y_or_N_String);

        if (FW_selection == 1)
        {
          u8g2.clear();
          u8g2.drawStr(2, u8g2.getMaxCharHeight() * 3 + 1, "Firmware Flash Mode!");
          u8g2.sendBuffer();
          // Ok go back to where the function was called from This option works because where this error is called
          //__disable_irq(); // Dont disable inttrupts or the clocks can init after the jump out
          *bootloader_flag = BOOTLOADER_FLAG_VALUE;
          NVIC_SystemReset();
        }
        if (FW_selection == 2)
        {
          // Head back to Main meanu
          // Abs_pos_error = true;
          MAIN_MENU();
        }
      }
      if (Sub_selection == 6)
      {
        // Head back to Main meanu
        MAIN_MENU();
      }
    }

    // else if
  }
}

void setup()
{
  // put the initlization code here.
  /// Setup Innterupts
  attachInterrupt(digitalPinToInterrupt(Motor0LimitSw), x1HomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor1LimitSw), y1HomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor2LimitSw), y2HomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor3LimitSw), y3HomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor4LimitSw), y4HomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor5LimitSw), aoatHomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor6LimitSw), aoabHomeIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Motor7LimitSw), x2HomeIsr, CHANGE);
  //// Setpper Driver Initilization
  driverX0.beginSerial(115200); // X driver Coms begin
  // Serial.println("Driver X Enabled\n");
  driverX0.begin();
  driverX0.rms_current(1100); // mA
  driverX0.microsteps(64);
  // driverX0.en_spreadCycle(0); // Page 44 use stealth chop
  driverX0.pwm_ofs_auto();
  driverX0.pwm_autograd(1);
  driverX0.pwm_autoscale(1);

  driverX1.beginSerial(115200);
  // Serial.println("Driver X2 Enabled\n");
  driverX1.begin();
  driverX1.rms_current(1100); // mA
  driverX1.microsteps(64);
  driverX1.pwm_ofs_auto();
  driverX1.pwm_autograd(1);
  driverX1.pwm_autoscale(1);

  driverY0.beginSerial(115200);
  // Serial.println("Driver Y0 Enabled\n");
  driverY0.begin();
  driverY0.rms_current(1100); // mA
  driverY0.microsteps(64);
  // driverY0.en_spreadCycle(0);
  driverY0.pwm_ofs_auto();
  driverY0.pwm_autoscale(1);
  driverY0.pwm_autograd(1);

  driverY1.beginSerial(115200);
  // Serial.println("Driver Y12 Enabled\n");
  driverY1.begin();
  driverY1.rms_current(900); // mA
  driverY1.microsteps(64);
  driverY1.pwm_ofs_auto();

  driverY2.beginSerial(115200);
  // Serial.println("Driver Y12 Enabled\n");
  driverY2.begin();
  driverY2.rms_current(900); // mA
  driverY2.microsteps(64);
  driverY2.pwm_ofs_auto();

  driverY3.beginSerial(115200);
  // Serial.println("Driver Y3 Enabled\n");
  driverY3.begin();
  driverY3.rms_current(850); // mA
  driverY3.microsteps(64);
  driverY3.pwm_ofs_auto();

  driverAOAT.beginSerial(115200);
  // Serial.println("driver e1 enabled\n");
  driverAOAT.begin();
  driverAOAT.rms_current(900); // ma
  driverAOAT.microsteps(64);
  driverAOAT.pwm_ofs_auto();

  driverAOAB.beginSerial(115200);
  // Serial.println("Driver E2 Enabled\n");
  driverAOAB.begin();
  driverAOAB.rms_current(900); // mA
  driverAOAB.microsteps(64);
  driverAOAB.pwm_ofs_auto();
}

void SET_ACELL(float x, float y, float E0, float E1)
{
  X_stepper.setAccelerationInRevolutionsPerSecondPerSecond(x * 2);
  X2_stepper.setAccelerationInRevolutionsPerSecondPerSecond(x * 2);
  Y0_stepper.setAccelerationInRevolutionsPerSecondPerSecond(y * 2);
  Y1_stepper.setAccelerationInRevolutionsPerSecondPerSecond(y * 2); // By Default the Z axis will allways be attached to the Y (Verticle Axis)
  Y2_stepper.setAccelerationInRevolutionsPerSecondPerSecond(y * 2);
  Y3_stepper.setAccelerationInRevolutionsPerSecondPerSecond(y * 2);
  AOAT_stepper.setAccelerationInRevolutionsPerSecondPerSecond(E0); // Change to mm/s when the system is being implmented
  AOAB_stepper.setAccelerationInRevolutionsPerSecondPerSecond(E1); // Change to mm/s when the system is being implmented
  //  E2stepper.setAccelerationInStepsPerSecondPerSecond(*put axis its mirrioring here*);
  Acell_Data[0] = x;
  Acell_Data[1] = y;
  Acell_Data[2] = E0;
  Acell_Data[3] = E1;
}
void SET_SPEED(int x, int y, int E0, int E1)
{
  X_stepper.setSpeedInRevolutionsPerSecond(x * 2); // multpied by the leadscrew pitch because we are in rev/s and the value entered is in mm/s
  X2_stepper.setSpeedInRevolutionsPerSecond(x * 2);
  Y0_stepper.setSpeedInRevolutionsPerSecond(y * 2);
  Y1_stepper.setSpeedInRevolutionsPerSecond(y * 2);
  Y2_stepper.setSpeedInRevolutionsPerSecond(y * 2);
  Y3_stepper.setSpeedInRevolutionsPerSecond(y * 2); // Change to mm/s^2 when the system is being implmented
  AOAT_stepper.setSpeedInRevolutionsPerSecond(E0);  // Change to mm/s^2 when the system is being implmented ?
  AOAB_stepper.setSpeedInRevolutionsPerSecond(E1);
  //  E2stepper.setSpeedInStepsPerSecond(*Axis its mirrioring here*);
  Speed_Data[0] = x;
  Speed_Data[1] = y;
  Speed_Data[2] = E0;
  Speed_Data[3] = E1;
}

void x1HomeIsr()
{
  x1home = !x1home; // set set them as hommed when the homing function is called
}
void x2HomeIsr()
{
  x2home = !x1home; // set set them as hommed when the homing function is called
}
void y1HomeIsr()
{
  y1home = !y1home;
}
void y2HomeIsr()
{
  y2home = !y2home;
}
void y3HomeIsr()
{
  y3home = !y3home;
}
void y4HomeIsr()
{
  y4home = !y4home;
}
void aoatHomeIsr()
{
  aoathome = !aoathome;
}
void aoabHomeIsr()
{
  aoabhome = !aoabhome;
}
void motionTriggerIsr()
{
  // Go_Pressed = true;
}
void estopIsr()
{
  NVIC_SystemReset(); // use a software reset to kill the board
}

/* For As long As the Octopus Board is used under no circustances should this ever be modified !!!*/
/*
This section of code determines how the system clock is cofigured this is important for the
STM32F446ZET6 in this case our board runs at 168 MHz not the 8Mhz external clock the board expects by default
No need to understand, attempt to or even try to.
Include it in every version that is compiled form this platfrom IO envrioment/stm32dunio envrioment
*/

extern "C" void SystemClock_Config(void)
{
//#ifdef OCTOPUS_BOARD
#ifdef OCTOPUS_BOARD_FROM_HSI
  /* boot from HSI, internal 16MHz RC, to 168MHz. **NO USB POSSIBLE**, needs HSE! */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
#else
  /* boot from HSE, crystal oscillator (12MHz) */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLQ;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#endif
  // #else
  //   /* nucleo board, 8MHz external clock input, HSE in bypass mode */
  //   RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  //   RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  //   RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  //   /** Configure the main internal regulator output voltage
  //    */
  //   __HAL_RCC_PWR_CLK_ENABLE();
  //   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  //   /** Initializes the RCC Oscillators according to the specified parameters
  //    * in the RCC_OscInitTypeDef structure.
  //    */
  //   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  //   RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  //   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  //   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  //   RCC_OscInitStruct.PLL.PLLM = 4;
  //   RCC_OscInitStruct.PLL.PLLN = 168;
  //   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  //   RCC_OscInitStruct.PLL.PLLQ = 7;
  //   RCC_OscInitStruct.PLL.PLLR = 2;
  //   if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  //   {
  //     Error_Handler();
  //   }
  //   /** Initializes the CPU, AHB and APB buses clocks
  //    */
  //   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  //   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  //   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  //   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  //   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  //   if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  //   {
  //     Error_Handler();
  //   }
  //   PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
  //   PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLQ;
  //   if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  //   {
  //     Error_Handler();
  //   }
  // #endif
}