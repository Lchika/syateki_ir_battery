#ifndef _IR_BATTERY_CONTROL_ADC_H_
#define _IR_BATTERY_CONTROL_ADC_H_

typedef struct{
  int average;
  int min;
  int max;
} adc_result;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief initialize adc pin setting
 * @param (pin_number) set as A0 => 0, A1 => 1,...
 * @retval (bool) false: error occurred
 */
bool adc_init(int pin_number);

/**
 * @brief read analog input
 * @param (pin_number) set as A0 => 0, A1 => 1,...
 * @retval (int) 16bit analog input
 */
bool analog_read(int pin_number, adc_result *result);

bool adc_term(void);

#endif // _IR_BATTERY_CONTROL_ADC_H_