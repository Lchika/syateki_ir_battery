#ifndef _IR_BATTERY_CONTROL_SERVO_SONTROL_H_
#define _IR_BATTERY_CONTROL_SERVO_SONTROL_H_

#include <nuttx/arch.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool servo_init(int channel);

bool servo_set_angle(int channel, double angle);

bool servo_term(void);

#endif // _IR_BATTERY_CONTROL_SERVO_SONTROL_H_