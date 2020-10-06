#ifndef _IR_BATTERY_CONTROL_DISTANCE_CALC_H_
#define _IR_BATTERY_CONTROL_DISTANCE_CALC_H_

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int distance_calc_config(unsigned int save_size, double min_value, double max_value);
double distance_calc_update(int handler, double data);
bool distance_calc_term(int handler);

#endif    // _IR_BATTERY_CONTROL_DISTANCE_CALC_H_