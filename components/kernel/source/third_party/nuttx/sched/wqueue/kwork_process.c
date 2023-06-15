/****************************************************************************
 * sched/wqueue/kwork_process.c
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

#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <nuttx/queue.h>

#include <nuttx/wqueue.h>

#include <sys/times.h>

#include "wqueue.h"

#ifdef CONFIG_SCHED_WORKQUEUE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Use CLOCK_MONOTONIC if it is available.  CLOCK_REALTIME can cause bad
 * delays if the time is changed.
 */

#ifdef CONFIG_CLOCK_MONOTONIC
#  define WORK_CLOCK CLOCK_MONOTONIC
#else
#  define WORK_CLOCK CLOCK_REALTIME
#endif

#ifdef CONFIG_SYSTEM_TIME64
#  define WORK_DELAY_MAX portMAX_DELAY
#else
#  define WORK_DELAY_MAX portMAX_DELAY
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_process
 *
 * Description:
 *   This is the logic that performs actions placed on any work list.  This
 *   logic is the common underlying logic to all work queues.  This logic is
 *   part of the internal implementation of each work queue; it should not
 *   be called from application level logic.
 *
 * Input Parameters:
 *   wqueue - Describes the work queue to be processed
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void work_process(FAR struct kwork_wqueue_s *wqueue, int wndx, const EventBits_t waitbit)
{
  volatile FAR struct work_s *work;
  worker_t  worker;
  FAR void *arg;
  clock_t elapsed;
  clock_t remaining;
  clock_t stick;
  clock_t ctick;
  clock_t next;
  struct tms tms;

  /* Then process queued work.  We need to keep interrupts disabled while
   * we process items in the work list.
   */

  next  = WORK_DELAY_MAX;
  taskENTER_CRITICAL();

  /* Get the time that we started processing the queue in clock ticks. */

  stick = times(&tms);

  /* And check each entry in the work queue.  Since we have disabled
   * interrupts we know:  (1) we will not be suspended unless we do
   * so ourselves, and (2) there will be no changes to the work queue
   */

  work = (FAR struct work_s *)wqueue->q.head;
  while (work != NULL)
    {
      /* Is this work ready?  It is ready if there is no delay or if
       * the delay has elapsed. qtime is the time that the work was added
       * to the work queue.  It will always be greater than or equal to
       * zero.  Therefore a delay of zero will always execute immediately.
       */

      ctick   = times(&tms);
      elapsed = ctick - work->qtime;
      if (elapsed >= work->delay)
        {
          /* Remove the ready-to-execute work from the list */

          dq_rem((struct dq_entry_s *)work, &wqueue->q);

          /* Extract the work description from the entry (in case the work
           * instance by the re-used after it has been de-queued).
           */

          worker = work->worker;

          /* Check for a race condition where the work may be nullified
           * before it is removed from the queue.
           */

          if (worker != NULL)
            {
              /* Extract the work argument (before re-enabling interrupts) */

              arg = work->arg;

              /* Do the work.  Re-enable interrupts while the work is being
               * performed... we don't have any idea how long this will take!
               */

              work->doing = true;

              /* Mark the work as no longer being queued */
              work->worker = NULL;

              taskEXIT_CRITICAL();

              vTaskSetThreadLocalStoragePointer(NULL, configTHREAD_LOCAL_STORAGE_POINTER_IDX_WORK, (void *)work);

              worker(arg);

              vTaskSetThreadLocalStoragePointer(NULL, configTHREAD_LOCAL_STORAGE_POINTER_IDX_WORK, (void *)NULL);

              taskENTER_CRITICAL();

              if (work->doing == true)
                {
                  complete((struct completion *)&(work->done));
                }

              /* Now, unfortunately, since we re-enabled interrupts we don't
               * know the state of the work list and we will have to start
               * back at the head of the list.
               */

              work->doing = false;

              work  = (FAR struct work_s *)wqueue->q.head;
            }
          else
            {
              /* Cancelled.. Just move to the next work in the list with
               * interrupts still disabled.
               */

              work = (FAR struct work_s *)work->dq.flink;
            }
        }
      else /* elapsed < work->delay */
        {
          /* This one is not ready.
           *
           * NOTE that elapsed is relative to the current time,
           * not the time of beginning of this queue processing pass.
           * So it may need an adjustment.
           */

          elapsed += (ctick - stick);
          if (elapsed > work->delay)
            {
              /* The delay has expired while we are processing */

              elapsed = work->delay;
            }

          /* Will it be ready before the next scheduled wakeup interval? */

          remaining = work->delay - elapsed;
          if (remaining < next)
            {
              /* Yes.. Then schedule to wake up when the work is ready */

              next = remaining;
            }

          /* Then try the next in the list. */

          work = (FAR struct work_s *)work->dq.flink;
        }
    }

  wqueue->worker[wndx].busy = false;
  taskEXIT_CRITICAL();
  complete(&(wqueue->worker[wndx].completion));
  xEventGroupWaitBits(g_work_event, waitbit, pdTRUE,
                      pdFALSE, next);
  wqueue->worker[wndx].busy = true;
}

#endif /* CONFIG_SCHED_WORKQUEUE */
