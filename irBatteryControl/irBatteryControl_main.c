#include <sdk/config.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "debug_print.h"
#include "analog_read.h"
#include "servo_control.h"
#include "camera_main.h"
//#include "TM1637.h"
#include "distance_calc.h"

/* Include worker header */
#include "displayCamera.h"

/* Worker ELF path */
#define WORKER_FILE "/mnt/spif/displayCamera"

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

#define message(format, ...)    printf(format, ##__VA_ARGS__)
#define err(format, ...)        fprintf(stderr, format, ##__VA_ARGS__)

#define HIGH 1
#define LOW  0

#define ANALOG_INPUT_NUM 4

#define BASE_SERVO_MIN   10     // RIGHT
#define BASE_SERVO_MAX  170     // LEFT
#define UPPER_SERVO_MIN  50     // UP
#define UPPER_SERVO_MAX 110     // DOWN
#define SERVO_DIFF_DEF   0.1
#define SERVO_DIFF_MAX   1

typedef enum { SERVO_RIGHT = 0, SERVO_LEFT, SERVO_UP, SERVO_DOWN, SERVO_NONE } ServoDirection;

static double convert_analog2distance(double analog_in){
  // https://blog.obniz.io/blog/%E8%B5%A4%E5%A4%96%E7%B7%9A%E8%B7%9D%E9%9B%A2%E3%82%BB%E3%83%B3%E3%82%B5%E3%83%BC-gp2y0a21yk0f-2/
  //return 19988.34 * pow(((analog_in + 32767) / 65534) * 1024, -1.25214) * 10;
  return 26.549 * pow((((double)analog_in + 32767.0) / 65534.0 * 5.0), -1.2091);
}

static int map(double val, double min1, double max1, int min2, int max2){
  return (int)(min2 + (max2 - min2) * ((val - min1) / (max1 - min1)));
}

int irBatteryControl_main(int argc, char *argv[])
{
  mptask_t mptask;
  mpmutex_t mutex;
  mpshm_t shm;
  mpmq_t mq;
  uint32_t msgdata;
  int data = 0x1234;
  int ret, wret;
  char *buf;

  /* Initialize MP task */

  ret = mptask_init(&mptask, WORKER_FILE);
  if (ret != 0)
    {
      err("mptask_init() failure. %d\n", ret);
      return ret;
    }

  ret = mptask_assign(&mptask);
  if (ret != 0)
    {
      err("mptask_assign() failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP mutex and bind it to MP task */

  ret = mpmutex_init(&mutex, DISPLAYCAMERAKEY_MUTEX);
  if (ret < 0)
    {
      err("mpmutex_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &mutex);
  if (ret < 0)
    {
      err("mptask_bindobj(mutex) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP message queue with assigned CPU ID, and bind it to MP task */

  ret = mpmq_init(&mq, DISPLAYCAMERAKEY_MQ, mptask_getcpuid(&mptask));
  if (ret < 0)
    {
      err("mpmq_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &mq);
  if (ret < 0)
    {
      err("mptask_bindobj(mq) failure. %d\n", ret);
      return ret;
    }

  /* Initialize MP shared memory and bind it to MP task */

  ret = mpshm_init(&shm, DISPLAYCAMERAKEY_SHM, 1024);
  if (ret < 0)
    {
      err("mpshm_init() failure. %d\n", ret);
      return ret;
    }
  ret = mptask_bindobj(&mptask, &shm);
  if (ret < 0)
    {
      err("mptask_binobj(shm) failure. %d\n", ret);
      return ret;
    }

  /* Map shared memory to virtual space */

  buf = mpshm_attach(&shm, 0);
  if (!buf)
    {
      err("mpshm_attach() failure.\n");
      return ret;
    }
  message("attached at %08x\n", (uintptr_t)buf);
  memset(buf, 0, 1024);

  /* Run worker */

  ret = mptask_exec(&mptask);
  if (ret < 0)
    {
      err("mptask_exec() failure. %d\n", ret);
      return ret;
    }

  /* Send command to worker */
  ret = mpmq_send(&mq, MSG_ID_DISPLAYCAMERA, (uint32_t) &data);
  if (ret < 0)
    {
      err("mpmq_send() failure. %d\n", ret);
      return ret;
    }
  
  servo_init(0);
  servo_init(1);

  pthread_t pthread_camera;
  pthread_create(&pthread_camera, NULL, &camera_main, NULL);
  
  // init analog read pin
  if(!adc_init(0)){
    ERROR_PRINTF("faild to init a0 pin");
    return -1;
  }
  if(!adc_init(1)){
    ERROR_PRINTF("faild to init a1 pin");
    return -1;
  }
  if(!adc_init(2)){
    ERROR_PRINTF("faild to init a2 pin");
    return -1;
  }
  if(!adc_init(3)){
    ERROR_PRINTF("faild to init a3 pin");
    return -1;
  }
  if(!adc_init(4)){
    ERROR_PRINTF("faild to init a4 pin");
    return -1;
  }

  /*
  const unsigned int distance_save_size = 8;
  const double min_distance = 20.0;
  const double max_distance = 50.0;
  int a0_distance_handler = distance_calc_config(distance_save_size, min_distance, max_distance);
  if(a0_distance_handler == -1){
    ERROR_PRINTF("faild to a0 distance_calc_config()");
    return -1;
  }
  int a1_distance_handler = distance_calc_config(distance_save_size, min_distance, max_distance);
  if(a1_distance_handler == -1){
    ERROR_PRINTF("faild to a1 distance_calc_config()");
    return -1;
  }
  */
  
  uint32_t pin_led[ANALOG_INPUT_NUM] = {PIN_HIF_IRQ_OUT, PIN_PWM3, PIN_SPI2_MOSI, PIN_SPI3_CS1_X};
  for(int i = 0; i < sizeof(pin_led) / sizeof(pin_led[0]); i++){
    board_gpio_config(pin_led[i], 0, false, false, PIN_FLOAT);
    board_gpio_write(pin_led[i], LOW);
  }
  
  bool is_active = true;
  //TM1637_init(PIN_SPI3_CS1_X, PIN_SPI2_MOSI, TM1637_BRIGHT_TYPICAL);

  double a0_angle = (BASE_SERVO_MAX + BASE_SERVO_MIN) / 2.0;
  double a1_angle = 85;
  servo_set_angle(0, a0_angle);
  servo_set_angle(1, a1_angle);
  float servo_diff = SERVO_DIFF_DEF;
  while(is_active){
    // get speed ratio from potentiomater
    adc_result speed_ratio_adc;

    if(!analog_read(4, &speed_ratio_adc)){
      ERROR_PRINTF("faild to read a4");
    }
    DEBUG_PRINTF("a4: %d", speed_ratio_adc.average);
    servo_diff = SERVO_DIFF_MAX * (((float)speed_ratio_adc.average + 32767.0f) / 65534.0f);
    DEBUG_PRINTF("servo_diff: %f", servo_diff);

    // get analog value from distance sensor
    adc_result a_result[ANALOG_INPUT_NUM];

    for(int i = 0; i < ANALOG_INPUT_NUM; i++){
      if(!analog_read(i, &a_result[i])){
        ERROR_PRINTF("faild to read a%d", i);
      }
    }

    DEBUG_PRINTF("a0 ave: %d, a1 ave: %d, a2 ave: %d, a3 ave: %d",
                 a_result[0].average, a_result[1].average, a_result[2].average, a_result[3].average);
    
    double a_distance[ANALOG_INPUT_NUM] = {99.0, 99.0, 99.0, 99.0};
    for(int i = 0; i < ANALOG_INPUT_NUM; i++){
      a_distance[i] = convert_analog2distance((double)a_result[i].average);
    }

    DEBUG_PRINTF("Distance[cm] a0: %.3lf, a1: %.3lf, a2: %.3lf, a3: %.3lf",
                 a_distance[0], a_distance[1], a_distance[2], a_distance[3]);

    //a_distance[0] = distance_calc_update(a0_distance_handler, a_distance[0]);
    //a_distance[1] = distance_calc_update(a1_distance_handler, a_distance[1]);
    
    //DEBUG_PRINTF("a0 smooth: %.3lf, a1 smooth: %.3lf", a_distance[0], a_distance[1]);
    
    //TM1637_displayNum((float)a0_distance, 0, false);
    
    ServoDirection servo_direction = SERVO_NONE;
    for(int i = 0; i < sizeof(a_distance) / sizeof(a_distance[0]); i++){
      if(a_distance[i] < 30.0){
        if(servo_direction == SERVO_NONE){
          servo_direction = i;
        }else{
          servo_direction = SERVO_NONE;
          break;
        }
      }
    }
    
    for(int i = 0; i < sizeof(pin_led) / sizeof(pin_led[0]); i++){
      board_gpio_write(pin_led[i], LOW);
    }
    
    switch(servo_direction){
      case SERVO_RIGHT:
        DEBUG_PRINTF("servo move RIGHT: direction = %d, base_angle = %.2lf, upper_angle = %.2lf", servo_direction, a0_angle, a1_angle);
        a0_angle = (a0_angle - servo_diff) < BASE_SERVO_MIN ? BASE_SERVO_MIN : a0_angle - servo_diff;
        servo_set_angle(0, a0_angle);
        board_gpio_write(pin_led[0], HIGH);
        break;
      case SERVO_LEFT:
        DEBUG_PRINTF("servo move LEFT: direction = %d, base_angle = %.2lf, upper_angle = %.2lf", servo_direction, a0_angle, a1_angle);
        a0_angle = (a0_angle + servo_diff) > BASE_SERVO_MAX ? BASE_SERVO_MAX : a0_angle + servo_diff;
        servo_set_angle(0, a0_angle);
        board_gpio_write(pin_led[1], HIGH);
        break;
      case SERVO_UP:
        DEBUG_PRINTF("servo move UP: direction = %d, base_angle = %.2lfd, upper_angle = %.2lf", servo_direction, a0_angle, a1_angle);
        a1_angle = (a1_angle - servo_diff) < UPPER_SERVO_MIN ? UPPER_SERVO_MIN : a1_angle - servo_diff;
        servo_set_angle(1, a1_angle);
        board_gpio_write(pin_led[2], HIGH);
        break;
      case SERVO_DOWN:
        DEBUG_PRINTF("servo move DOWN: direction = %d, base_angle = %.2lf, upper_angle = %.2lf", servo_direction, a0_angle, a1_angle);
        a1_angle = (a1_angle + servo_diff) > UPPER_SERVO_MAX ? UPPER_SERVO_MAX : a1_angle + servo_diff;
        servo_set_angle(1, a1_angle);
        board_gpio_write(pin_led[3], HIGH);
        break;
      default:
        break;
    }
    
    // 100ms sleep
    usleep(100 * 1000);
  }

  /* Wait for worker message */

  ret = mpmq_receive(&mq, &msgdata);
  if (ret < 0)
    {
      err("mpmq_recieve() failure. %d\n", ret);
      return ret;
    }
  message("Worker response: ID = %d, data = %x\n",
          ret, *((int *)msgdata));

  /* Show worker copied data */

  /* Lock mutex for synchronize with worker after it's started */

  mpmutex_lock(&mutex);

  message("Worker said: %s\n", buf);

  mpmutex_unlock(&mutex);

  /* Destroy worker */

  wret = -1;
  ret = mptask_destroy(&mptask, false, &wret);
  if (ret < 0)
    {
      err("mptask_destroy() failure. %d\n", ret);
      return ret;
    }

  message("Worker exit status = %d\n", wret);

  adc_term();

  /* Finalize all of MP objects */

  mpshm_detach(&shm);
  mpshm_destroy(&shm);
  mpmutex_destroy(&mutex);
  mpmq_destroy(&mq);

  pthread_join(pthread_camera, NULL);

  return 0;
}
