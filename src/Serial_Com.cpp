#include <Arduino.h>
#include <U8g2lib.h>
#include <TMCStepper.h>
#include <Serial_Com.hpp>


/*
 Varibles that need to be handled in the converstion to c++:
    newData
    receivedChars[ndx]

*/

// LCD class im not sure how to bring this class in correctly so i am just definigh it everhwere it is needed.
static U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/PE13, /* data=*/PE15, /* CS=*/PE14, /* reset=*/PE10);

// TMC Stepper Class
static TMC2209Stepper driverX0(PC4, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
static TMC2209Stepper driverX1(PE1, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
static TMC2209Stepper driverY0(PD11, PA6, .11f, DRIVER_ADDRESS);  // (RX, TX,RSENSE, Driver address) Software serial X axis
static TMC2209Stepper driverY1(PC6, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
static TMC2209Stepper driverY2(PD3, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver address) Software serial X axis
static TMC2209Stepper driverY3(PC7, PA6, .11f, DRIVER_ADDRESS);   // (RX, TX,RSENSE, Driver Address) Software serial X axis
static TMC2209Stepper driverAOAT(PF2, PA6, .11f, DRIVER_ADDRESS); // (RX, TX,RESENSE, Driver address) Software serial X axis
static TMC2209Stepper driverAOAB(PE4, PA6, .11f, DRIVER_ADDRESS); // (RX, TX,RESENSE, Driver address) Software serial X axis
// TMC2209Stepper driverE3(PE1, PA6, .11f, DRIVER_ADDRESS );  // (RX, TX,RESENSE, Driver address) Software serial X axi

// ~~~~~~~~~Serial Read Functions~~~~~~~~~~~~~~~~~~~
bool recvWithStartEndMarkers(bool newData, char receivedChars[], byte numChars)
{
  static bool printedMsg = 0; // Debug
  if (printedMsg == 0)        // Debug
  {
    Serial.println("Got to revrecvWithStartEndMarkers()\n"); // Debug output only print once
    printedMsg = 1;                                          // Debug
  }
  // Serial.println("Got to revrecvWithStartEndMarkers()\n");
  static bool recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;
  while (Serial.available() > 0 && newData == false)
  {
    // Serial.println("Got to while (Serial.available() > 0 && newData == false) in recvWithStartEndMarkers()"); // Debug output
    rc = Serial.read();         // Look at the next character
    if (recvInProgress == true) // if we are recording
    {
      if (rc != endMarker) // And we are not at the end marker
      {
        receivedChars[ndx] = rc; // Throw the current char into the array
        ndx++;                   // increment index forward.
        if (ndx >= numChars)     // If we exceed the max continue to read and just throw the data into the last postition
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;    // stop recording
        ndx = 0;                   // set index back to zero (fromaility not truly required)
        bool newData = true;            // Let the program know that there is data wating for the parser.
      }
    }
    else if (rc == startMarker) // If Rc is the start marker we are getting good data
    {
      recvInProgress = true; // Start recording
    }
  }
  return newData,receivedChars;
}

/*
The following is a custom G/M code implmentation
// We are pasing in Data like this:
// <G X###.## Y###.## AoAT###.## AoAB###.##>
// OR
// <M A X###.## Y###.## AoAT###.## AoAB###.##> or <M S X###.## Y###.## AoAT###.## AoAB###.##>
// Temp Charters (charter array) looks like:
// G X###.## Y###.## AoAT###.## AoAB###.##
// OR
// M A X###.## Y###.## AoAT###.## AoAB###.##
// OR
// M S X###.## Y###.## AoAT###.## AoAB###.##
Any combination of capital/lowercase letters or single varibles are accepted and should be handled.
For Example:
// M S X#### OR M S Y ###### are acceptable inputs
// G X100 or G X100 Y 200 are acceptable too
For futher understanding, write out your own string and walk through the code below
*/
bool parseData()
{ // split the data into its parts
  Serial.println("Got to parse data\n");
  char *strtokIndx; // this is used by strtok() as an index
  // char * LeadChar; // Getting the lead Charter of the command
  float Temp_Pos[5] = {Xpos, Ypos, AoA[0], AoA[1], NULL};
  int Setting_Num;      // Acelleration = 0 Speed = 1
  int Temp_Settings[5]; // only passed into the global varible if this function completes sucessfully

  // strtok(tempChars, " ");// get the first part - the string // This returns the first token "G"
  strtokIndx = strtok(tempChars, " "); // get the first part - the string // This returns the first token "G"
  Serial.println(strtokIndx[0]);
  if (strtokIndx[0] == 'R' || strtokIndx[0] == 'r')
  {
    // Simulated Estop AkA Just Kill power to the board
    // digitalWrite(Reset,LOW); // Lmao This is one way to skin the cat rather than bothering with software
  }
  if (strtokIndx[0] == 'G' || strtokIndx[0] == 'g')
  {                                         // Begin G code parsing
    Serial.println("IT Starts With A G\n"); // G code Section
    // Movement parsing goes in here
    strtokIndx = strtok(NULL, " "); // process next string segment // This returns the next token "X##.##" then "Y##.##"...
    Serial.println(strtokIndx[0]);
    if (strtokIndx[0] == 'H' || strtokIndx[0] == 'h')
    { // if the first character is X
      Serial.println("IT has an h\n");
      strtokIndx = strtok(NULL, " ");
      Serial.println(strtokIndx[0]);
      if (strtokIndx[0] == 'A')
      {
        // If there is nothing after H then Home all axis here
        Serial.println("heading to home all");
        HomeAll();
      }
      else
      {
        while (strtokIndx != NULL)
        {
          if (strtokIndx[0] == 'X' || strtokIndx[0] == 'x')
          { // if the first character is X
            LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_13);
            while (x1home == false)
            { // While they arent hit the end stop we move the motors
              if (x1home == false)
              {
                LL_GPIO_TogglePin(GPIOF, LL_GPIO_PIN_13);
              }
              delayMicroseconds(2);
              LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_13);
              delayMicroseconds(200);
            }
            Xpos = 0;
            volatile bool x1home = false;
            return true;
          }
          if (strtokIndx[0] == 'Y' || strtokIndx[0] == 'y')
          { // if the first character is Y
            // Home The Y
            LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_0);
            LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_11);
            LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_4);
            while (y1home == false)
            {
              if (y1home == false)
              {
                // motorgpiog=motorgpiog-0b0000000000000001; // remove pg0
                // motorgpiof=motorgpiof-0b0000100000000000; // remove pf11
                LL_GPIO_TogglePin(GPIOG, LL_GPIO_PIN_0);
                LL_GPIO_TogglePin(GPIOF, LL_GPIO_PIN_11);
                LL_GPIO_TogglePin(GPIOG, LL_GPIO_PIN_4);
              }
              delayMicroseconds(2);
              LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_0);
              LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_11);
              LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_4);
              delayMicroseconds(110);
            }
            Serial.print("Finshed y Homing");
            Ypos = 0;
            volatile bool yhome = false;
            return true;
          }
          if (strtokIndx[0] == 'A' || strtokIndx[0] == 'a')
          { // if the first character is A -> Meaning AoA
            if (strtokIndx[3] == 'T' || strtokIndx[3] == 't')
            { // if the third character is T -> Meaning AoAT
              // Home AoA Top here
              LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_9);
              while (aoathome == false)
              {
                if (aoathome == false)
                {
                  // motorgpiog=motorgpiog-0b0000000000010000;
                  LL_GPIO_TogglePin(GPIOF, LL_GPIO_PIN_9);
                }
                delayMicroseconds(2);
                LL_GPIO_ResetOutputPin(GPIOF, LL_GPIO_PIN_9);
                delayMicroseconds(200);
              }
              AoA[0] = 0;
              volatile bool aoathome = false;
              return true;
            }
            if (strtokIndx[3] == 'B' || strtokIndx[3] == 'b')
            { // if the third character is B -> Meaning AoAB
              // Home AoA Bottom here
              LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);
              while (aoabhome == false)
              {
                if (aoabhome == false)
                {
                  // motorgpiog=motorgpiog-0b0000000000010000;
                  LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13);
                }
                delayMicroseconds(2); // delay between high and low (Aka how long the pin is high)
                LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);
                delayMicroseconds(200);
              }
              AoA[1] = 0;
              volatile bool aoabhome = false;
              return true;
            }
          }
        } // end while
      }   // End else
    }
    else if (isdigit(strtokIndx[0]))
    {
      Serial.print("G-code entered does not match the correct format please try again when prompted\n");
      strtokIndx = NULL; // exit the while loop
      return false;      // tell the system this command failed
    }
    else
    {
      // If it Gets here its in the Form <G X##.##>
      while (strtokIndx != NULL)
      {
        Serial.println(strtokIndx[0]);
        if (strtokIndx[0] == 'X' || strtokIndx[0] == 'x')
        { // if the first character is X
          Serial.println("IT has an X\n");
          char *substr = strtokIndx + 1; // This Truncates the first char "X"
          Temp_Pos[0] = atof(substr);    // Assign the value to the X position
        }
        if (strtokIndx[0] == 'Y' || strtokIndx[0] == 'y')
        {                                // if the first character is Y
          char *substr = strtokIndx + 1; // This Truncates the first char "X"
          Temp_Pos[1] = atof(substr);    // Assign the value to the Y position
        }
        if (strtokIndx[0] == 'A' || strtokIndx[0] == 'a')
        { // if the first character is A -> Meaning AoA
          if (strtokIndx[3] == 'T' || strtokIndx[3] == 't')
          {                                // if the third character is T -> Meaning AoAT
            char *substr = strtokIndx + 4; // This Truncates the chacters "AoAT"
            Temp_Pos[2] = atof(substr);    // Assign the value to the AoAT position
          }
          if (strtokIndx[3] == 'B' || strtokIndx[3] == 'b')
          {                                // if the third character is B -> Meaning AoAB
            char *substr = strtokIndx + 4; // This Truncates the chacters "AoAB"
            Temp_Pos[3] = atof(substr);    // Assign the value to the AoAB position
          }
        }
        if (strtokIndx == NULL)
        {
          Serial.print("G-code entered does not match the correct format please try again when prompted\n");
          strtokIndx = NULL; // exit the while loop
          return false;      // tell the system this command failed
        }

        strtokIndx = strtok(NULL, " "); // process next string segment // This returns the next token "X##.##" then "Y##.##"...
        // if(strtokIndx[0] == NULL)
        // {
        //   Serial.println("strtokIndx[0] is NULL\n");
        // }
        // else
        // {
        //   Serial.println("Something else happened. I don't know what's going on.\n");
        // }
        // We are incrementing at the end of the loop so it stops before it could fall into the else statment when it reaches the end of the string
      }                 // End While loop
    }                   // If it makes it out of the while loop with out getting kicked out
    Xpos = Temp_Pos[0]; // set the global varibles
    Ypos = Temp_Pos[1];
    AoA[0] = Temp_Pos[2];
    AoA[1] = Temp_Pos[3];
    // Serial.println(Xpos);
    // Serial.println(Ypos);
    // Serial.println(AoA[0]); // Debug code here
    // Serial.println(AoA[1]);
    // Serial.println("Heading to \"MOVE_FUNCTION()\".");
    MOVE_FUNCTION();
    return true; // Tell the system that the function worked
  }              // End G code parsing
                 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MCODE Section ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                 // M A X###.## Y###.## AoAT###.## AoAB###.## or M S X###.## Y###.## AoAT###.## AoAB###.##
  if (strtokIndx[0] == 'M' || strtokIndx[0] == 'm')
  { // If the First Char is M
    Serial.println(("strtokIndx[0] == M or m; strtokIndx[0] == " + std::string(strtokIndx)).c_str());
    strtokIndx = strtok(NULL, " "); // Get the second token "A or S"
    Serial.println(("Got next strtokIndx; strtokIndx[0] == " + std::string(strtokIndx)).c_str());
    if (strtokIndx[0] == 'A' || strtokIndx[0] == 'a')
    {
      Serial.println("strtokIndx[0] == A or a");
      Setting_Num = 0; // i intoduced this varible because we will lose the A as this is a singly linked list (Forward)
    }
    else if (strtokIndx[0] == 'S' || strtokIndx[0] == 's')
    {
      Serial.println("strtokIndx[0] == S or a");
      Setting_Num = 1;
    }
    else if (strtokIndx[0] == 'D' || strtokIndx[0] == 'D')
    {
      Serial.println("strtokIndx[0] == D or d");
      strtokIndx = strtok(NULL, " "); // if it starts with a D get the next one
      Serial.println("strtokIndx = strtok() called.");
      if (strtokIndx[0] == 'M' || strtokIndx[0] == 'm')
      {
        Serial.println("strtokIndx[0] == M or m");
        Setting_Num = 2;
      }
      if (strtokIndx[0] == 'P' || strtokIndx[0] == 'p')
      {
        Serial.println("strtokIndx[0] == P or p");
        Setting_Num = 3;
      }
      if (strtokIndx[0] == 'S' || strtokIndx[0] == 's')
      {
        Serial.println("strtokIndx[0] == S or s");
        Setting_Num = 4;
      }
    }
    else
    {
      Serial.print("M-code entered does not match the correct format please try again when prompted");
      return false; // tell the system this command failed
    }
    strtokIndx = strtok(NULL, " "); // process next string segment // This returns the next token "X##.##" then "Y##.##"...
    Serial.println(("strtokIndx == " + std::string(strtokIndx)).c_str());
    if (isdigit(strtokIndx[0]))
    {
      Serial.println("Got to assigning numbers");
      Temp_Settings[0] = atof(strtokIndx); // Setting is for all Axis
      Temp_Settings[1] = atof(strtokIndx);
      Temp_Settings[2] = atof(strtokIndx);
      Temp_Settings[3] = atof(strtokIndx);
    }
    else
    {
      Serial.println("Not a digit; About to enter temp-settings assignment while loop.");
      while (strtokIndx != NULL)
      {
        Serial.println("Entered temp-settings assignment while loop.");
        if (strtokIndx[0] == 'X' || strtokIndx[0] == 'x')
        { // if the first character is X
          Serial.println("Assign X");
          char *substr = strtokIndx + 1;   // This Truncates the first char "X"
          Temp_Settings[0] = atof(substr); // Assign the value to the X position
        }
        if (strtokIndx[0] == 'Y' || strtokIndx[0] == 'y')
        { // if the first character is Y
          Serial.println("Assign Y.");
          char *substr = strtokIndx + 1;   // This Truncates the first char "Y"
          Temp_Settings[1] = atof(substr); // Assign the value to the Y position
        }
        if (strtokIndx[0] == 'A' || strtokIndx[0] == 'a')
        { // if the first character is A -> Meaning AoA
          Serial.println("strtokIndx[0] == A or a");
          if (strtokIndx[3] == 'T' || strtokIndx[3] == 't')
          { // if the third character is T -> Meaning AoAT
            Serial.println("strtokIndx[0] == T or t");
            char *substr = strtokIndx + 4;   // This Truncates the chacters "AoAT"
            Temp_Settings[2] = atof(substr); // Assign the value to the AoAT position
          }
          if (strtokIndx[3] == 'B' || strtokIndx[3] == 'b')
          { // if the third character is B -> Meaning AoAB
            Serial.println("strtokIndx[0] == B or b");
            char *substr = strtokIndx + 4;   // This Truncates the chacters "AoAB"
            Temp_Settings[3] = atof(substr); // Assign the value to the AoAB position
          }
        }
        strtokIndx = strtok(NULL, " "); // process next string segment // This returns the next token "X##.##" then "Y##.##"...
      }                                 // End While loop
    }                                   // End Else Statment
    if (Setting_Num == 0)
    { // If Acceleration
      Serial.println("Set acceleration");
      Acell_Data[0] = Temp_Settings[0];
      Acell_Data[1] = Temp_Settings[1];
      Acell_Data[2] = Temp_Settings[2];
      Acell_Data[3] = Temp_Settings[3];
      SET_ACELL(Acell_Data[0], Acell_Data[1], Acell_Data[2], Acell_Data[3]);
      return true; // Tell the system that the function worked
    }
    if (Setting_Num == 1)
    { // Speed settings
      Serial.println("Set speed");
      Speed_Data[0] = Temp_Settings[0];
      Speed_Data[1] = Temp_Settings[1];
      Speed_Data[2] = Temp_Settings[2];
      Speed_Data[3] = Temp_Settings[3];
      SET_SPEED(Speed_Data[0], Speed_Data[1], Speed_Data[2], Speed_Data[3]);
      return true; // Tell the system that the function worked
    }
    if (Setting_Num == 2)
    { // Mirco stepping // 0,16,64,256
      Serial.println("Setting_Num == 2; Microstepping");
      if (Temp_Settings[0] != 0 || Temp_Settings[0] != 16 || Temp_Settings[0] != 64 || Temp_Settings[0] != 256 || Temp_Settings[1] != 0 || Temp_Settings[1] != 16 || Temp_Settings[1] != 64 || Temp_Settings[1] != 256 ||
          Temp_Settings[2] != 0 || Temp_Settings[2] != 16 || Temp_Settings[2] != 64 || Temp_Settings[2] != 256 || Temp_Settings[3] != 0 || Temp_Settings[3] != 16 || Temp_Settings[3] != 64 || Temp_Settings[3] != 256)
      {
        Serial.println("Microstepping not an acceptable input");
        return false; // The entered number was not one of the acctable inputs
      }
      else
      {
        Serial.println("Setting microstepping");
        Micro_stepping[0] = Temp_Settings[0];  // x
        Micro_stepping[1] = Temp_Settings[1];  // y
        Micro_stepping[2] = Micro_stepping[1]; // Z is tied to y
        Micro_stepping[3] = Temp_Settings[2];
        Micro_stepping[4] = Temp_Settings[3];
        driverX0.microsteps(Micro_stepping[0]);
        driverX1.microsteps(Micro_stepping[0]);
        driverY0.microsteps(Micro_stepping[1]);
        driverY1.microsteps(Micro_stepping[1]);
        driverY2.microsteps(Micro_stepping[1]);
        driverY3.microsteps(Micro_stepping[2]);
        driverAOAT.microsteps(Micro_stepping[3]);
        driverAOAB.microsteps(Micro_stepping[4]);
        return true; // Tell the system that the function worked
      }
    }
    if (Setting_Num == 3)
    { // Stepper Mode
      Serial.println("Setting_Num == 3");
      if (Temp_Settings[0] != 0 || Temp_Settings[0] != 1)
      {
        return false; // The entered number was one of the acctable inputs
      }
      else
      {
        driverX0.en_spreadCycle(Temp_Settings[0]);
        driverX1.en_spreadCycle(Temp_Settings[0]);
        driverY0.en_spreadCycle(Temp_Settings[1]);
        driverY1.en_spreadCycle(Temp_Settings[2]);
        driverY2.en_spreadCycle(Temp_Settings[2]);
        driverY3.en_spreadCycle(Temp_Settings[2]);
        driverAOAT.en_spreadCycle(Temp_Settings[3]);
        driverAOAB.en_spreadCycle(Temp_Settings[4]);
        return true; // Tell the system that the function worked
      }
    }
    if (Setting_Num == 4)
    { // Stall Gaurd
      if (Temp_Settings[0] != 0 || Temp_Settings[0] != 1)
      {
        return false; // The entered number was one of the acctable inputs
      }
      else
      {
        return true; // Tell the system that the function worked
      }
    }
  }
  else
  {
    Serial.println("Shouldn't Have made it here\n");
    return false;
  }
  return true;
} // End Parsing Function

void showParsedData()
/* This function is purely debug related and is not used for any functional purpose it can be commented out in code
At any time do not comment out this function or you will break the entire system.*/
{ // show parsed data and move
  Serial.println("Parsed Data Debug output");
  Serial.print("X Pos");
  Serial.println(Xpos);
  Serial.print("Y Pos"); // debug stuff here
  Serial.println(Ypos);
  Serial.print("\nAoA Top");
  Serial.println(AoA[0]);
  Serial.print("AoA Bottom ");
  Serial.println(AoA[1]);
  // Serial.print("AoA Bottom Speed");
  // Serial.println(Speed_Data[1]);
  // Serial.print("AoA Bottom Speed");
  // Serial.println(Acell_Data[1]);
  // MOVE_FUNCTION(); // Revmoved When this function was truned to debug
  //  move function goes here
}
void gui_output_function()
{
  /* Python GUI Function --> This function just prints the current postion over serial.
Thiis is so python GUI can read it and know where stepper is currently, It is also usefull for postioning debug It is ignored by the GUI using the
 "%" symbol -> That means Any Data passed to the gui after the % symbol and before aother % symbol will be ignored by the text output of the GUI.
 The data is serperated by X,Y,AT,AB  */
  Serial.print("%"); // Start the Data Transfer
  Serial.print("Q"); // print out the the current settings
  Serial.print(Xpos);
  Serial.print("W");
  Serial.print(Ypos);
  Serial.print("E");
  Serial.print(AoA[0]);
  Serial.print("R");
  Serial.print(AoA[1]);
  Serial.print("T"); // ax
  Serial.print(Acell_Data[0]);
  Serial.print("Y"); // ay
  Serial.print(Acell_Data[1]);
  Serial.print("U"); // at
  Serial.print(Speed_Data[2]);
  Serial.print("I"); // ab
  Serial.print(Acell_Data[3]);
  Serial.print("O"); // sx
  Serial.print(Speed_Data[0]);
  Serial.print("P"); // sy
  Serial.print(Speed_Data[1]);
  Serial.print("A"); // st
  Serial.print(Speed_Data[2]);
  Serial.print("S"); // sb
  Serial.print(Speed_Data[3]);
  Serial.print("%"); // End Data transfer.

  Serial.print("X"); // print out the the current settings debug stuff
  Serial.print(Xpos);
  Serial.print("Y");
  Serial.print(Ypos);
  Serial.print("T");
  Serial.print(AoA[0]);
  Serial.print("B");
  Serial.print(AoA[1]);
  Serial.print("Q");
  Serial.print(Acell_Data[0]);
  Serial.print("W");
  Serial.print(Acell_Data[1]);
  Serial.print("E");
  Serial.print(Speed_Data[2]);
  Serial.print("R");
  Serial.print(Acell_Data[3]);
  Serial.print("A");
  Serial.print(Speed_Data[0]);
  Serial.print("S");
  Serial.print(Speed_Data[1]);
  Serial.print("D");
  Serial.print(Speed_Data[2]);
  Serial.print("F");
  Serial.print(Speed_Data[3]);
}
void serial_flush_buffer()
{
  while (Serial.read() >= 0)
    ; // do nothing
  Serial.print("Serial Flushed\n");
}