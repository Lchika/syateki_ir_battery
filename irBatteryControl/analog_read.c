/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/arch.h>

#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <arch/chip/scu.h>
#include <arch/chip/adc.h>

#include "debug_print.h"
#include "analog_read.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define ADC_PIN_MAX 6
#define ADC_LPADC0_PATH "/dev/lpadc0"
#define ADC_LPADC1_PATH "/dev/lpadc1"
#define ADC_LPADC2_PATH "/dev/lpadc2"
#define ADC_LPADC3_PATH "/dev/lpadc3"
#define ADC_HPADC0_PATH "/dev/hpadc0"
#define ADC_HPADC1_PATH "/dev/hpadc1"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bool is_initialized[ADC_PIN_MAX];
static char *devpath_list[ADC_PIN_MAX] = {ADC_LPADC0_PATH,
                                          ADC_LPADC1_PATH,
                                          ADC_LPADC2_PATH,
                                          ADC_LPADC3_PATH,
                                          ADC_HPADC0_PATH,
                                          ADC_HPADC1_PATH};
static int fd_list[ADC_PIN_MAX];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool adc_init(int pin_number){
  if(pin_number < 0 || pin_number >= ADC_PIN_MAX){
    ERROR_PRINTF("invalid pin number: %d, valid range is [%d %d]"
                 , pin_number, 0, ADC_PIN_MAX);
    return false;
  }

  if(is_initialized[pin_number]){
    ERROR_PRINTF("aleready initialized, pin = %d", pin_number);
    return true;
  }
  
  fd_list[pin_number] = open(devpath_list[pin_number], O_RDONLY);
  if(fd_list[pin_number] < 0){
    ERROR_PRINTF("open %s faild: %d", devpath_list[pin_number], errno);
    return false;
  }

  // SCU FIFO overwrite
  int ret = ioctl(fd_list[pin_number], SCUIOC_SETFIFOMODE, 1);
  if(ret < 0){
    ERROR_PRINTF("ioctl(SETFIFOMODE) failed: %d", errno);
    close(fd_list[pin_number]);
    return false;
  }

  // Start A/D conversion
  ret = ioctl(fd_list[pin_number], ANIOC_CXD56_START, 0);
  if(ret < 0){
    ERROR_PRINTF("ioctl(START) faild: %d", errno);
    close(fd_list[pin_number]);
    return false;
  }

  // wait
  //up_mdelay(1000);
  usleep(1000 * 1000);

  is_initialized[pin_number] = true;
  return true;
}


bool analog_read(int pin_number, adc_result *result){
  int bufsize = 16;
  char *buftop = (char *)malloc(bufsize);
  if(!buftop){
    ERROR_PRINTF("malloc faild. size:%d", bufsize);
    return false;
  }

  if(fd_list[pin_number] < 0){
    ERROR_PRINTF("open %s faild: %d", devpath_list[pin_number], errno);
    if(buftop){
      free(buftop);
    }
    return false;
  }

  // read Data
  int nbytes = read(fd_list[pin_number], buftop, bufsize);

  if(nbytes < 0){
    ERROR_PRINTF("read faild:%d", errno);
    if(buftop){
      free(buftop);
    }
    return false;
  }else if(nbytes == 0){
    DEBUG_PRINTF("read data size = %d", nbytes);
  }else{
    if(nbytes & 1){
      DEBUG_PRINTF("read size=%ld is not a multiple of sample size = %d",
                   (long)nbytes, sizeof(uint16_t));
    }else{
      int32_t count = 0;
      char *start = buftop;
      char *end = buftop + bufsize;

      if(bufsize > nbytes){
        end = buftop + nbytes;
      }

      int sum = 0, min = 0, max = 0;
      while(1){
        int16_t data = (int16_t)(*(uint16_t *)(start));
        min = ((min == 0) || (data < min)) ? data : min;
        max = ((max == 0) || (data > max)) ? data : max;
        sum += (int32_t)data;
        count++;
        start += sizeof(uint16_t);
        if(start >= end){
          break;
        }
      }

      if(count > 0){
        result->average = sum / count;
        result->min = min;
        result->max = max;
      }
    }
  }
  
  if(buftop){
    free(buftop);
  }
  return true;
}

bool adc_term(void){
  bool is_succeed = true;

  // stop A/D conversion
  for(int i = 0; i < ADC_PIN_MAX; i++){
    if(is_initialized[i]){
      int ret = ioctl(fd_list[i], ANIOC_CXD56_STOP, 0);
      if(ret < 0){
        ERROR_PRINTF("ioctl(STOP) faild: %d", errno);
        is_succeed = false;
      }
      close(fd_list[i]);
      is_initialized[i] = false;
    }
  }
  return is_succeed;
}