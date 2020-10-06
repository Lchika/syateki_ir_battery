#ifndef _IR_BATTERY_CONTROL_TM1637_H_
#define _IR_BATTERY_CONTROL_TM1637_H_

#define TM1637_BRIGHT_DARKEST 0
#define TM1637_BRIGHT_TYPICAL 2
#define TM1637_BRIGHTEST      7

void TM1637_init(int pin_clk, int pin_data, int bright);
void TM1637_displayNum(float num, int decimal, bool show_minus);

#endif //_IR_BATTERY_CONTROL_TM1637_H_