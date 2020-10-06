/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/timers/pwm.h>

#include <sys/ioctl.h>

#include <fcntl.h>

#include "servo_control.h"
#include "debug_print.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define PWM_PIN_MAX 4
#define PWM0_PATH "/dev/pwm0"
#define PWM1_PATH "/dev/pwm1"
#define PWM2_PATH "/dev/pwm2"
#define PWM3_PATH "/dev/pwm3"

#define FREQ_DEFALUT 50


/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct pwm_info_s g_pwm_info[PWM_PIN_MAX];
static bool is_initialized[PWM_PIN_MAX];
static char *devpath_list[PWM_PIN_MAX] = {PWM0_PATH,
                                          PWM1_PATH,
                                          PWM2_PATH,
                                          PWM3_PATH};
static int fd_list[PWM_PIN_MAX];

/****************************************************************************
 * Public Functions
 ****************************************************************************/
bool servo_init(int channel){
  if(channel < 0 || channel >= PWM_PIN_MAX){
    ERROR_PRINTF("invalid channel: %d, valid range is [%d %d]\n",
                 channel, 0, PWM_PIN_MAX);
    return false;
  }

  if(is_initialized[channel]){
    return true;
  }

  fd_list[channel] = open(devpath_list[channel], O_RDONLY);
  if(fd_list[channel] < 0){
    ERROR_PRINTF("open %s faild: %d\n", devpath_list[channel], errno);
    return false;
  }

  g_pwm_info[channel].frequency = FREQ_DEFALUT;
  g_pwm_info[channel].duty = ((uint32_t)(((90. / 180. * 2000) + 500) / 20000. * 100) << 16) / 100;
  int ret = ioctl(fd_list[channel], PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&g_pwm_info[channel]));
  if(ret < 0){
    ERROR_PRINTF("ioctl(PWMIOC_SETCHARACTERISTICS) faild: %d\n", errno);
    close(fd_list[channel]);
    return false;
  }
  
  ret = ioctl(fd_list[channel], PWMIOC_START, 0);
  if(ret < 0){
    ERROR_PRINTF("ioctl(PWMIOC_START) faild: %d\n", errno);
    close(fd_list[channel]);
    return false;
  }

  is_initialized[channel] = true;
  return true;
}

bool servo_set_angle(int channel, double angle){
  if(channel < 0 || channel >= PWM_PIN_MAX){
    ERROR_PRINTF("invalid channel: %d, valid range is [%d %d]\n",
                 channel, 0, PWM_PIN_MAX);
    return false;
  }

  if(fd_list[channel] < 0){
    ERROR_PRINTF("open %s faild: %d\n", devpath_list[channel], errno);
    return false;
  }

  g_pwm_info[channel].duty = (uint32_t)(((angle / 180. * 2000) + 500) / 20000. * 65535);
  int ret = ioctl(fd_list[channel], PWMIOC_SETCHARACTERISTICS, (unsigned long)((uintptr_t)&g_pwm_info[channel]));
  if(ret < 0){
    ERROR_PRINTF("ioctl(PWMIOC_SETCHARACTERISTICS) faild: %d\n", errno);
    return false;
  }

  return true;
}

bool servo_term(void){
  bool is_succeed = true;

  for(int i = 0; i < PWM_PIN_MAX; i++){
    if(is_initialized[i]){
      int ret = ioctl(fd_list[i], PWMIOC_STOP, 0);
      if(ret < 0){
        ERROR_PRINTF("ioctl(PWMIOC_STOP) faild: %d\n", errno);
        is_succeed = false;
      }
      close(fd_list[i]);
      is_initialized[i] = false;
    }
  }

  return is_succeed;
}