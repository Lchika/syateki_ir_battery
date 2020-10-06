#include <sdk/config.h>

#include <unistd.h>
#include <math.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "TM1637.h"

#define HIGH 1
#define LOW  0

#define TM1637_POINT_ON  1
#define TM1637_POINT_OFF 0
#define ADDR_AUTO 0x40
#define ADDR_FIXED 0x44
#define DIGITS 4

typedef struct {
  uint32_t pin_clk;
  uint32_t pin_data;
} TM1637_PIN_INFO;

static TM1637_PIN_INFO pin_info = {-1, -1};
static bool _point_flag = TM1637_POINT_OFF;
static int8_t _cmd_disp_ctrl = 0x88 + TM1637_BRIGHT_TYPICAL;

static int8_t tube_tab[] = {0x3f, 0x06, 0x5b, 0x4f,
                            0x66, 0x6d, 0x7d, 0x07,
                            0x7f, 0x6f, 0x77, 0x7c,
                            0x39, 0x5e, 0x79, 0x71
                           }; //0~9,A,b,C,d,E,F

static uint8_t char2segments(char c) {
  switch (c) {
    case '_' : return 0x08;
    case '^' : return 0x01; // ¯
    case '-' : return 0x40;
    case '*' : return 0x63; // °
    case ' ' : return 0x00; // space
    case 'A' : return 0x77; // upper case A
    case 'a' : return 0x5f; // lower case a
    case 'B' :              // lower case b
    case 'b' : return 0x7c; // lower case b
    case 'C' : return 0x39; // upper case C
    case 'c' : return 0x58; // lower case c
    case 'D' :              // lower case d
    case 'd' : return 0x5e; // lower case d
    case 'E' :              // upper case E
    case 'e' : return 0x79; // upper case E
    case 'F' :              // upper case F
    case 'f' : return 0x71; // upper case F
    case 'G' :              // upper case G
    case 'g' : return 0x35; // upper case G
    case 'H' : return 0x76; // upper case H
    case 'h' : return 0x74; // lower case h
    case 'I' : return 0x06; // 1
    case 'i' : return 0x04; // lower case i
    case 'J' : return 0x1e; // upper case J
    case 'j' : return 0x16; // lower case j
    case 'K' :              // upper case K
    case 'k' : return 0x75; // upper case K
    case 'L' :              // upper case L
    case 'l' : return 0x38; // upper case L
    case 'M' :              // twice tall n
    case 'm' : return 0x37; // twice tall ∩
    case 'N' :              // lower case n
    case 'n' : return 0x54; // lower case n
    case 'O' :              // lower case o
    case 'o' : return 0x5c; // lower case o
    case 'P' :              // upper case P
    case 'p' : return 0x73; // upper case P
    case 'Q' : return 0x7b; // upper case Q
    case 'q' : return 0x67; // lower case q
    case 'R' :              // lower case r
    case 'r' : return 0x50; // lower case r
    case 'S' :              // 5
    case 's' : return 0x6d; // 5
    case 'T' :              // lower case t
    case 't' : return 0x78; // lower case t
    case 'U' :              // lower case u
    case 'u' : return 0x1c; // lower case u
    case 'V' :              // twice tall u
    case 'v' : return 0x3e; // twice tall u
    case 'W' : return 0x7e; // upside down A
    case 'w' : return 0x2a; // separated w
    case 'X' :              // upper case H
    case 'x' : return 0x76; // upper case H
    case 'Y' :              // lower case y
    case 'y' : return 0x6e; // lower case y
    case 'Z' :              // separated Z
    case 'z' : return 0x1b; // separated Z
  }
  return 0;
}

static int8_t coding(int8_t disp_data) {
  if (disp_data == 0x7f) {
    disp_data = 0x00;    // Clear digit
  } else if (disp_data >= 0 && disp_data < (int)(sizeof(tube_tab) / sizeof(*tube_tab))) {
    disp_data = tube_tab[disp_data];
  } else if (disp_data >= '0' && disp_data <= '9') {
    disp_data = tube_tab[(int)disp_data - 48];    // char to int (char "0" = ASCII 48)
  } else {
    disp_data = char2segments(disp_data);
  }
  disp_data += _point_flag == TM1637_POINT_ON ? 0x80 : 0;

  return disp_data;
}

static void start(void){
  board_gpio_write(pin_info.pin_clk, HIGH);
  board_gpio_write(pin_info.pin_data, HIGH);
  board_gpio_write(pin_info.pin_data, LOW);
  board_gpio_write(pin_info.pin_clk, LOW);
}

static void bitDelay(void){
  usleep(50);
}

static int writeByte(int8_t wr_data) {
  for (uint8_t i = 0; i < 8; i++) { // Sent 8bit data
    board_gpio_write(pin_info.pin_clk, LOW);

    if (wr_data & 0x01) {
      board_gpio_write(pin_info.pin_data, HIGH);    // LSB first
    } else {
      board_gpio_write(pin_info.pin_data, LOW);
    }
    wr_data >>= 1;
    board_gpio_write(pin_info.pin_clk, HIGH);
  }

  board_gpio_write(pin_info.pin_clk, LOW);    // Wait for the ACK
  board_gpio_write(pin_info.pin_data, HIGH);
  board_gpio_write(pin_info.pin_clk, HIGH);
  board_gpio_config(pin_info.pin_data, 0, true, false, PIN_FLOAT);

  bitDelay();
  int ack = board_gpio_read(pin_info.pin_data);

  if (ack == 0) {
    board_gpio_config(pin_info.pin_data, 0, false, false, PIN_FLOAT);
    board_gpio_write(pin_info.pin_data, LOW);
  }

  bitDelay();
  board_gpio_config(pin_info.pin_data, 0, false, false, PIN_FLOAT);
  bitDelay();

  return ack;
}

static void stop(void) {
  board_gpio_write(pin_info.pin_clk, LOW);
  board_gpio_write(pin_info.pin_data, LOW);
  board_gpio_write(pin_info.pin_clk, HIGH);
  board_gpio_write(pin_info.pin_data, HIGH);
}

static void display(uint8_t bit_addr, int8_t disp_data){
  int8_t seg_data;

  seg_data = coding(disp_data);
  start();               // Start signal sent to TM1637 from MCU
  writeByte(ADDR_FIXED); // Command1: Set data
  stop();
  start();
  writeByte(bit_addr | 0xc0); // Command2: Set data (fixed address)
  writeByte(seg_data);        // Transfer display data 8 bits
  stop();
  start();
  writeByte(_cmd_disp_ctrl); // Control display
  stop();
}

static void clear_diaplay(void){
  display(0x00, 0x01);
  display(0x01, 0x01);
  display(0x02, 0x01);
  display(0x03, 0x7f);
}

static void point(bool PointFlag) {
  _point_flag = PointFlag;
}

void TM1637_init(int pin_clk, int pin_data, int bright){
  pin_info.pin_clk = pin_clk;
  pin_info.pin_data = pin_data;

  board_gpio_config(pin_clk, 0, false, false, PIN_FLOAT);
  board_gpio_config(pin_data, 0, false, false, PIN_FLOAT);

  clear_diaplay();

  _cmd_disp_ctrl = 0x88 + bright;
  return;
}

void TM1637_displayNum(float num, int decimal, bool show_minus){
  // Displays number with decimal places (no decimal point implementation)
  // Colon is used instead of decimal point if decimal == 2
  // Be aware of int size limitations (up to +-2^15 = +-32767)

  int number = round(fabs(num) * pow(10, decimal));

  for (int i = 0; i < DIGITS - (show_minus && num < 0 ? 1 : 0); ++i) {
    int j = DIGITS - i - 1;

    if (number != 0) {
      display(j, number % 10);
    } else {
      display(j, 0x7f);    // display nothing
    }

    number /= 10;
  }

  if (show_minus && num < 0) {
    display(0, '-');    // Display '-'
  }

  if (decimal == 2) {
    point(true);
  } else {
    point(false);
  }
}