/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MGOS_MOCK_H
#define __MGOS_MOCK_H

/* Some functions mocked from MGOS, so we can run unit tests standalone.
 */

#include "mgos.h"

// mgos_log
enum cs_log_level {
  LL_NONE          = -1,
  LL_ERROR         = 0,
  LL_WARN          = 1,
  LL_INFO          = 2,
  LL_DEBUG         = 3,
  LL_VERBOSE_DEBUG = 4,

  _LL_MIN          = -2,
  _LL_MAX          = 5,
};

int log_print_prefix(enum cs_log_level l, const char *func, const char *file);

#define LOG(l, x)                                              \
  do {                                                         \
    if (log_print_prefix(l, __func__, __FILE__)) { printf x; } \
    printf("\r\n");                                            \
  } while (0)


// mgos_timer
#define MGOS_TIMER_REPEAT    1
typedef uintptr_t mgos_timer_id;
typedef void (*timer_callback)(void *param);

mgos_timer_id mgos_set_timer(int msecs, int flags, timer_callback cb, void *cb_arg);
void mgos_clear_timer(mgos_timer_id id);

#endif // __MGOS_MOCK_H
