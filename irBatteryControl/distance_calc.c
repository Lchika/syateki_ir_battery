#include <sdk/config.h>

#include <stdlib.h>
#include <string.h>

#include "debug_print.h"
#include "distance_calc.h"

#define HANDLER_MAX_NUM 5

static bool is_initialized = false;
static double *_saved_data[HANDLER_MAX_NUM] = {};
static unsigned int _save_size[HANDLER_MAX_NUM] = {};
static double _min_value[HANDLER_MAX_NUM] = {};
static double _max_value[HANDLER_MAX_NUM] = {};
static unsigned int _next_index[HANDLER_MAX_NUM] = {};

int distance_calc_config(unsigned int save_size, double min_value, double max_value){
  DEBUG_PRINTF("save_size: %u, min_value: %.3lf, max_value: %.3lf", save_size, min_value, max_value);
  if(!is_initialized){
    for(int i = 0; i < HANDLER_MAX_NUM; i++){
      _saved_data[i] = NULL;
    }
    is_initialized = true;
  }

  int handler = -1;
  for(int i = 0; i < HANDLER_MAX_NUM; i++){
    if(_saved_data[i] == NULL){
      handler = i;
      break;
    }
  }
  if(handler == -1){
    // no empty handler
    return handler;
  }

  if(_saved_data[handler] != NULL){
    free(_saved_data[handler]);
  }
  _saved_data[handler] = (double *)malloc(sizeof(double) * save_size);
  if(_saved_data[handler] == NULL){
    ERROR_PRINTF("faild to malloc saved_data (%d)", errno);
    return false;
  }
  memset(_saved_data[handler], 0, sizeof(double) * save_size);
  
  _save_size[handler] = save_size;
  _min_value[handler] = min_value;
  _max_value[handler] = max_value;
  _next_index[handler] = 0;
  
  return handler;
}

double distance_calc_update(int handler, double data){
  if(data < _min_value[handler]){
    data = _min_value[handler];
  }
  if(data > _max_value[handler]){
    data = _max_value[handler];
  }
  _saved_data[handler][_next_index[handler]] = data;
  
  DEBUG_PRINTF("--- _saved_data ---");
  for(int i = 0; i < _save_size[handler]; i++){
    DEBUG_PRINTF("[%d] %.3lf", i, _saved_data[handler][i]);
  }
  DEBUG_PRINTF("---             ---");

  if(_next_index[handler] >= _save_size[handler] - 1){
    _next_index[handler] = 0;
  }else{
    _next_index[handler]++;
  }
  double sum = 0.0;
  for(int i = 0; i < _save_size[handler]; i++){
    sum += _saved_data[handler][i];
  }
  return sum / _save_size[handler];
}

bool distance_calc_term(int handler){
  if(_saved_data[handler] != NULL){
    free(_saved_data[handler]);
  }
  _save_size[handler] = 0;
  _min_value[handler] = 0;
  _max_value[handler] = 0;
  _next_index[handler] = 0;
  return true;
}