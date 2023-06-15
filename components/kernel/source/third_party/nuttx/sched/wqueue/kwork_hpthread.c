/****************************************************************************
 * sched/wqueue/kwork_hpthread.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <nuttx/config.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <nuttx/queue.h>

#include <nuttx/wqueue.h>

#include "wqueue.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/vfs.h>
#include <kernel/module.h>

#define LOG_TAG "WQUEUE"
#include <kernel/elog.h>

#ifdef CONFIG_SCHED_HPWORK

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The state of the kernel mode, high priority work queue(s). */

struct hp_wqueue_s g_hpwork;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_hpthread
 *
 * Description:
 *   These are the worker threads that performs the actions placed on the
 *   high priority work queue.
 *
 *   These, along with the lower priority worker thread(s) are the kernel
 *   mode work queues (also build in the flat build).
 *
 *   All kernel mode worker threads are started by the OS during normal
 *   bring up.  This entry point is referenced by OS internally and should
 *   not be accessed by application logic.
 *
 * Input Parameters:
 *   argc, argv (not used)
 *
 * Returned Value:
 *   Does not return
 *
 ****************************************************************************/

static void work_hpthread(void *arg)
{
  int wndx = 0;
#if CONFIG_SCHED_HPNTHREADS > 1
  TaskHandle_t me = xTaskGetCurrentTaskHandle();
  int i;

  /* Find out thread index by search the workers in g_hpwork */

  for (wndx = 0, i = 0; i < CONFIG_SCHED_HPNTHREADS; i++)
    {
      if (g_hpwork.worker[i].pid == me)
        {
          wndx = i;
          break;
        }
    }

  DEBUGASSERT(i < CONFIG_SCHED_HPNTHREADS);
#endif

  /* Loop forever */

  for (; ; )
    {
      /* Then process queued work.  work_process will not return until: (1)
       * there is no further work in the work queue, and (2) signal is
       * triggered, or delayed work expires.
       */

      work_process((FAR struct kwork_wqueue_s *)&g_hpwork, wndx, HPWORK);
    }

  vTaskDelete(NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_start_highpri
 *
 * Description:
 *   Start the high-priority, kernel-mode worker thread(s)
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   The task ID of the worker thread is returned on success.  A negated
 *   errno value is returned on failure.
 *
 ****************************************************************************/

static int work_start_highpri(void)
{
  TaskHandle_t pid = NULL;
  int wndx;
  char name[32] = { 0 };

  /* Don't permit any of the threads to run until we have fully initialized
   * g_hpwork.
   */

  taskENTER_CRITICAL();

  /* Start the high-priority, kernel mode worker thread(s) */

  log_i("Starting high-priority kernel worker thread(s)\n");

  for (wndx = 0; wndx < CONFIG_SCHED_HPNTHREADS; wndx++)
    {
      snprintf(name, sizeof(name), "%s%d", HPWORKNAME, wndx);
      xTaskCreate( work_hpthread, name, CONFIG_SCHED_HPWORKSTACKSIZE,
                  NULL, CONFIG_SCHED_HPWORKPRIORITY, &pid );

      if (pid == NULL)
        {
          log_e("ERROR: kthread_create %d failed: %p\n", wndx, pid);
          taskEXIT_CRITICAL();
          return (int)pid;
        }

      g_hpwork.worker[wndx].pid  = pid;
      g_hpwork.worker[wndx].busy = true;
      init_completion(&(g_hpwork.worker[wndx].completion));
    }

  taskEXIT_CRITICAL();
  return 0;
}

module_system(hpwork, work_start_highpri, NULL, 0)

#endif /* CONFIG_SCHED_HPWORK */
