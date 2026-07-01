#pragma once

#include "stdint.h"

typedef enum {
  BSP_OK,
  BSP_ERR,
  BSP_ERR_NULL,
  BSP_ERR_INITED,
  BSP_ERR_NO_DEV,
  BSP_ERR_BUSY,
  BSP_ERR_TIMEOUT,
  BSP_ERR_FULL,
  BSP_ERR_EMPTY,
} bsp_status_t;

typedef struct {
  void (*fn)(void *);
  void *arg;
} bsp_callback_t;

void bsp_init(void);
