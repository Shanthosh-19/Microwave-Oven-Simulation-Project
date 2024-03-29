/*
 * File:   main.c
 * Author: shant
 *
 * Created on 20 September, 2023, 7:34 PM
 */



#include "main.h"


#pragma config WDTE = OFF //Watchdog timer disabled

int operation_flag = POWER_ON_SCREEN; // power on screen(This is declared globally, as we need this to be used in other function)
unsigned char sec = 0, min = 0, flag = 0; // 0 
static void init_config(void) {
        //Initializing CLCD module
        init_clcd();
        
        //Initializing MKP module
        init_matrix_keypad();
        
        // RC2 pin as a output
        FAN_DDR = 0;
        FAN = OFF; // Turn off the fan
        
        BUZZER_DDR = 0; // RC1 pin as a output pin
        BUZZER = OFF;
        
        // Initialization of timer 2
        init_timer2();
        PEIE = 1;
        GIE = 1;
}

void main(void) {
    init_config(); //calling initializing function
    
    unsigned char key;
    int reset_flag;
    while (1) {
        key = read_matrix_keypad(STATE);
        if(operation_flag == MENU_DISPLAY_SCREEN)
        {
        //SW-1 is pressed
        if(key == 1)
        {
            operation_flag = MICRO_MODE;
            reset_flag = MODE_RESET;
            clear_screen();
            clcd_print(" power = 900W   ", LINE2(0));
            __delay_ms(3000); //3 sec delay
            clear_screen();
        }
        else if(key == 2) // grill mode
        {
           operation_flag = GRILL_MODE;
           reset_flag = MODE_RESET;
           clear_screen();
        }
        else if(key == 3) // convection
        {
            operation_flag = CONVECTION_MODE;
            reset_flag = MODE_RESET;
            clear_screen();
        }
        else if(key == 4) //start
        {
            sec = 30;
            min = 0;
            FAN = ON;
            TMR2ON = ON;
            clear_screen();
            operation_flag = TIME_DISPLAY;
           
        }
     }
        else if(operation_flag == TIME_DISPLAY)
        {
          if(key == 4) // start
          {
              sec = sec + 30;
              //sec > 59, sec == 60 -> 1 min
              if(sec > 59)
              {
                min++; 
                sec = sec - 60;
              }
          }
          else if(key == 5)// pause
          {
              operation_flag = PAUSE;
          }
          else if(key == 6)// stop
          {
              operation_flag = STOP;
              clear_screen();
          }
        }
        else if(operation_flag == PAUSE)
        {
            if(key == 4) // start/resume
            {
                FAN = ON;
                TMR2ON = ON;
                operation_flag = TIME_DISPLAY;
            }    
        }
        
       switch(operation_flag)
       {
           case POWER_ON_SCREEN:
               power_on_screen();
               operation_flag = MENU_DISPLAY_SCREEN; // menu display screen
               clear_screen(); 
               break;
           case MENU_DISPLAY_SCREEN:
               menu_display_screen();
               break;
           case GRILL_MODE:
               set_time(key, reset_flag);
               break;
           case MICRO_MODE:
               set_time(key, reset_flag);
               break;
           case CONVECTION_MODE:
               if(flag == 0)
               {
                   set_temp(key, reset_flag);
                   if(flag == 1)// # key
                   {
                       clear_screen();
                      reset_flag = MODE_RESET;
                      continue;
                   }
               }
               else if(flag == 1)
               {
                   set_time(key, reset_flag);
               }
               break;
           case TIME_DISPLAY:
               time_display_screen();
               break;
           case PAUSE:
               FAN = OFF;
               TMR2ON = OFF;
               break;
           case STOP:
               FAN = OFF;
               TMR2ON = OFF;
               operation_flag = MENU_DISPLAY_SCREEN;
       }
       reset_flag = RESET_NOTHING;
    }
}

void time_display_screen(void)
{
    // Printing in LINE1
    clcd_print(" TIME =  ",LINE1(0));
    // Print min and sec
    // Printing min on the set time screen
    clcd_putch( min/10 + '0',LINE1(9));
    clcd_putch( min%10 + '0',LINE1(10));
    clcd_putch(':',LINE1(11));
    
    // Printing sec on the set time screen
    clcd_putch( sec/10 + '0',LINE1(12));
    clcd_putch( sec%10 + '0',LINE1(13));
    // Print options
    clcd_print(" 4.Start/Resume", LINE2(0));
    clcd_print(" 5.Pause ",LINE3(0));
    clcd_print(" 6.Stop ",LINE4(0));
    
    if((sec == 0) &&(min == 0))
    {
       clear_screen();
       clcd_print(" Time Up !!", LINE2(0));
       BUZZER = ON;
       __delay_ms(3000); //delay of 3sec
       clear_screen();
       BUZZER = OFF;
       FAN = OFF;
       TMR2ON = OFF;
       operation_flag = MENU_DISPLAY_SCREEN;
    }
}
void power_on_screen(void)
{
    unsigned char i;
    //printing bar on line 1
    for(i=0; i<16; i++)
    {
        clcd_putch(BAR, LINE1(i));
        __delay_ms(100);
    }
    //printing power on message on line 2 and line 3
    clcd_print("  Powering ON   ",LINE2(0));
    __delay_ms(100);
    clcd_print(" Microwave Oven ", LINE3(0));
    __delay_ms(100);
    //printing bar on line 4
    for(i=0; i<16; i++)
    {
        clcd_putch(BAR, LINE4(i));
        __delay_ms(100);
    }
    //We place delay for reading purpose
    __delay_ms(3000);
}

void clear_screen(void)
{
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_us(500);
}
void menu_display_screen(void)
{
    clcd_print("1.Micro",LINE1(0));
    clcd_print("2.Grill",LINE2(0));
    clcd_print("3.Convection",LINE3(0));
    clcd_print("4.Start",LINE4(0));
}
void set_temp(unsigned char key, int reset_flag)
{
    static unsigned char key_count, blink, temp;
    static int wait;
    if(reset_flag == MODE_RESET)
    {
        key_count = 0;
        wait = 0;
        blink = 0;
        temp = 0;
        flag = 0;
        key = ALL_RELEASED;
        clcd_print(" SET TEMP(oC)",LINE1(0));
        clcd_print(" TEMP = ", LINE2(0));
        // sec -> 0 to 59
        clcd_print("*:CLEAR  #:ENTER", LINE4(0));
    }
    
    // Reading the temp
    if((key != '*') && (key != '#') && (key != ALL_RELEASED))
    {
        // key = 1 2
        key_count++; // 1 2
        //1 2
        if(key_count <= 3)// reading numbers for temp
        {
            temp = temp*10 + key; // 2 digits
        }
    }
    
    else if(key == '*') // clear sec or min in micro mode
    {
        temp = 0;
        key_count = 0;
    }
    else if((key == '#') && (temp <=180)) //enter the key
    {
        clear_screen();
       // Pre-heating
        clcd_print("  Pre-Heating ", LINE1(0));
        clcd_print(" Time Rem. = ", LINE3(0));
        TMR2ON = 1; // Switching on the timer
        sec = 180;
        while(sec !=0) // when the sec == 0, we will come out of the loop
        {
            // Printing sec on the pre-heating screen
            clcd_putch(sec/100 + '0', LINE3(11)); // 8th place--1
            clcd_putch((sec/10)%10 + '0', LINE3(12)); // 9th place--2
            clcd_putch((sec%10) + '0', LINE3(13)); // 10th place--3
        }
        //sec = 0
        if(sec == 0)
        {
            flag = 1;
            TMR2ON = 0;
              
        }
    }
    if(wait++ == 15)
    {
        wait = 0;
        blink = !blink;
        // Printing temp on the set temp screen
        clcd_putch(temp/100 + '0', LINE2(8)); // 8th place--1
        clcd_putch((temp/10)%10 + '0', LINE2(9)); // 9th place--2
        clcd_putch((temp%10) + '0', LINE2(10)); // 10th place--3
    }
    
    if(blink)
    {
       clcd_print("   ", LINE2(8));
         
    }
    
}
void set_time(unsigned char key, int reset_flag)
{
    static unsigned char key_count, blink_pos, blink;
    static int wait;
    if(reset_flag == MODE_RESET)
    {
        key_count = 0;
        sec = 0;
        min = 0;
        blink_pos = 0; // sec
        wait = 0;
        blink = 0;
        key = ALL_RELEASED;
        clcd_print("SET TIME (MM:SS)",LINE1(0));
        clcd_print("TIME-", LINE2(0));
        // sec -> 0 to 59
        clcd_print("*:CLEAR  #:ENTER", LINE4(0));
    }
    
    if((key != '*') && (key != '#') && (key != ALL_RELEASED))
    {
        // key = 1 2
        key_count++; // 1 2
        if(key_count <= 2)// reading numbers for seconds
        {
            sec = sec*10 + key; // 2 digits
            blink_pos = 0;
        }
        else if((key_count > 2) && (key_count <= 4))
        {
            min = min*10 + key;
            blink_pos = 1;
        }
    }
    else if(key == '*') // clear sec or min in micro mode
    {
      if(blink_pos == 0)
      {
          sec = 0;
          key_count = 0;
      }
      else if(blink_pos == 1)
      {
          min = 0;
          key_count = 2;
      }
    }
    else if(key == '#') //enter the key
    {
       clear_screen();
       operation_flag = TIME_DISPLAY;
       FAN = ON;
       //Switching ON timer 2
       TMR2ON = ON;
    }
    if(wait++ == 15)
    {
        wait = 0;
        blink = !blink;
        // Printing min on the set time screen
        clcd_putch( min/10 + '0',LINE2(6));
        clcd_putch( min%10 + '0',LINE2(7));
        clcd_putch(':',LINE2(8));
    
        // Printing sec on the set time screen
        clcd_putch( sec/10 + '0',LINE2(9));
        clcd_putch( sec%10 + '0',LINE2(10));
    }
    
    if(blink)
    {
        switch(blink_pos)
        {
            case 0: // sec
                clcd_print("  ", LINE2(9));
                break;
            case 1: //min
                clcd_print("  ", LINE2(6));
                break;
        }
        
    }
        
}