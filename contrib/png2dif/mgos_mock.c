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

/* Some functions mocked from MGOS, so we can run unit tests standalone.
 */

#include "mgos_mock.h"

int _mgos_timers = 0;

int log_print_prefix(enum cs_log_level l, const char *func, const char *file) {
  char ll_str[6];

  switch (l) {
  case LL_ERROR:
    strncpy(ll_str, "ERROR", sizeof(ll_str));
    break;

  case LL_WARN:
    strncpy(ll_str, "WARN", sizeof(ll_str));
    break;

  case LL_INFO:
    strncpy(ll_str, "INFO", sizeof(ll_str));
    break;

  case LL_DEBUG:
    strncpy(ll_str, "DEBUG", sizeof(ll_str));
    break;

  case LL_VERBOSE_DEBUG:
    strncpy(ll_str, "VERB", sizeof(ll_str));
    break;

  default:   // LL_NONE
    return 0;
  }
  printf("%-5s %-15s %-40s| ", ll_str, file, func);
  return 1;
}

mgos_timer_id mgos_set_timer(int msecs, int flags, timer_callback cb, void *cb_arg) {
  _mgos_timers++;
  LOG(LL_INFO, ("Installing timer -- %d timers currently installed", _mgos_timers));
  (void)msecs;
  (void)flags;
  (void)cb;
  (void)cb_arg;

  return _mgos_timers;
}

void mgos_clear_timer(mgos_timer_id id) {
  _mgos_timers--;
  LOG(LL_INFO, ("Clearing timer -- %d timers currently installed", _mgos_timers));
  (void)id;

  return;
}
