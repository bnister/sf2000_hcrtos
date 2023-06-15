/****************************************************************************
 * sched/wqueue/kwork_queue.c
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
#include <nuttx/queue.h>
#include <assert.h>
#include <errno.h>

#include <kernel/module.h>

#include <nuttx/wqueue.h>
#include <sys/times.h>

#include "wqueue.h"

#ifdef CONFIG_SCHED_WORKQUEUE

/****************************************************************************
 * Private Functions
 ****************************************************************************/

EventGroupHandle_t g_work_event = NULL;

/****************************************************************************
 * Name: work_qqueue
 *
 * Description:
 *   Queue work to be performed at a later time.  All queued work will be
 *   performed on the worker thread of execution (not the caller's).
 *
 *   The work structure is allocated by caller, but completely managed by
 *   the work queue logic.  The caller should never modify the contents of
 *   the work queue structure; the caller should not call work_qqueue()
 *   again until either (1) the previous work has been performed and removed
 *   from the queue, or (2) work_cancel() has been called to cancel the work
 *   and remove it from the work queue.
 *
 * Input Parameters:
 *   qid    - The work queue ID (index)
 *   work   - The work structure to queue
 *   worker - The worker callback to be invoked.  The callback will invoked
 *            on the worker thread of execution.
 *   arg    - The argument that will be passed to the workder callback when
 *            int is invoked.
 *   delay  - Delay (in clock ticks) from the time queue until the worker
 *            is invoked. Zero means to perform the work immediately.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void work_qqueue(FAR struct kwork_wqueue_s *wqueue,
                        FAR struct work_s *work, worker_t worker,
                        FAR void *arg, clock_t delay)
{
  struct tms tms;
  DEBUGASSERT(work != NULL && worker != NULL);

  /* Interrupts are disabled so that this logic can be called from with
   * task logic or ifrom nterrupt handling logic.
   */

  taskENTER_CRITICAL();

  /* Is there already pending work? */

  if (work->worker != NULL && work->doing == false)
    {
      /* Remove the entry from the work queue.  It will re requeued at the
       * end of the work queue.
       */

      dq_rem((FAR dq_entry_t *)work, &wqueue->q);
    }

  /* Initialize the work structure. */

  if (pvTaskGetThreadLocalStoragePointer(NULL, configTHREAD_LOCAL_STORAGE_POINTER_IDX_WORK) == (void *)work)
    {
      complete((struct completion *)&(work->done));
    }

  work->worker = worker;           /* Work callback. non-NULL means queued */
  work->arg    = arg;              /* Callback argument */
  work->delay  = delay;            /* Delay until work performed */
  work->doing  = false;
  init_completion(&work->done);

  /* Now, time-tag that entry and put it in the work queue */

  work->qtime  = times(&tms); /* Time work queued */

  dq_addlast((FAR dq_entry_t *)work, &wqueue->q);

  taskEXIT_CRITICAL();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_queue
 *
 * Description:
 *   Queue kernel-mode work to be performed at a later time.  All queued
 *   work will be performed on the worker thread of execution (not the
 *   caller's).
 *
 *   The work structure is allocated and must be initialized to all zero by
 *   the caller.  Otherwise, the work structure is completely managed by the
 *   work queue logic.  The caller should never modify the contents of the
 *   work queue structure directly.  If work_queue() is called before the
 *   previous work as been performed and removed from the queue, then any
 *   pending work will be canceled and lost.
 *
 * Input Parameters:
 *   qid    - The work queue ID (index)
 *   work   - The work structure to queue
 *   worker - The worker callback to be invoked.  The callback will invoked
 *            on the worker thread of execution.
 *   arg    - The argument that will be passed to the workder callback when
 *            int is invoked.
 *   delay  - Delay (in clock ticks) from the time queue until the worker
 *            is invoked. Zero means to perform the work immediately.
 *
 * Returned Value:
 *   Zero on success, a negated errno on failure
 *
 ****************************************************************************/

int work_queue(int qid, FAR struct work_s *work, worker_t worker,
               FAR void *arg, clock_t delay)
{
  /* Queue the new work */
  BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;

#ifdef CONFIG_SCHED_RPCWORK
  if (qid == RPCWORK)
    {
      /* Queue high priority work */
      work_qqueue((FAR struct kwork_wqueue_s *)&g_rpcwork, work, worker,
                  arg, delay);

     if (xPortIsInISR())
       {
          xResult = xEventGroupSetBitsFromISR(g_work_event, RPCWORK, &xHigherPriorityTaskWoken);
          if (xResult == pdPASS)
            {
              portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
       }
     else
       {
         xEventGroupSetBits(g_work_event, RPCWORK);
       }
      return OK;
    }
  else
#endif
#ifdef CONFIG_SCHED_HPWORK
  if (qid == HPWORK)
    {
      /* Queue high priority work */
      work_qqueue((FAR struct kwork_wqueue_s *)&g_hpwork, work, worker,
                  arg, delay);

     if (xPortIsInISR())
       {
          xResult = xEventGroupSetBitsFromISR(g_work_event, HPWORK, &xHigherPriorityTaskWoken);
          if (xResult == pdPASS)
            {
              portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
       }
     else
       {
         xEventGroupSetBits(g_work_event, HPWORK);
       }
      return OK;
    }
  else
#endif
#ifdef CONFIG_SCHED_LPWORK
  if (qid == LPWORK)
    {
      /* Queue low priority work */

      work_qqueue((FAR struct kwork_wqueue_s *)&g_lpwork, work, worker,
                  arg, delay);
      if (xPortIsInISR())
        {
          xResult = xEventGroupSetBitsFromISR(g_work_event, LPWORK, &xHigherPriorityTaskWoken);
          if (xResult == pdPASS)
            {
              portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
      else
        {
          xEventGroupSetBits(g_work_event, LPWORK);
        }
      return OK;
    }
  else
#endif
    {
      return -EINVAL;
    }
}

static int workqueue_initialize(void)
{
  g_work_event = xEventGroupCreate();
  return 0;
}

module_core(workqueue, workqueue_initialize, NULL, 0)

#endif /* CONFIG_SCHED_WORKQUEUE */
