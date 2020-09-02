/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _RTOS_H_
#define _RTOS_H_

#include "definitions.h"

#define doNothing()                     do { } while(0)

#if defined(SIMU)
  #include <pthread.h>
  #if __GNUC__
    #include <unistd.h>
    inline void msleep(unsigned x)
    {
      usleep(1000 * x);
    }
  #else
    #include <windows.h>
    #define msleep(x) Sleep(x)
  #endif
  #define RTOS_MS_PER_TICK              1
  #define RTOS_INIT()
  #define RTOS_WAIT_MS(x)               msleep(x)
  #define RTOS_WAIT_TICKS(x)            msleep((x)*2)
  #define RTOS_START()                  doNothing()
  #define RTOS_TASK_HANDLE              pthread_t
  #define RTOS_MUTEX_HANDLE             pthread_mutex_t
  #define RTOS_FLAG_HANDLE              uint32_t
  #define RTOS_EVENT_HANDLE             sem_t *
  #define RTOS_CREATE_MUTEX(mutex)      mutex = PTHREAD_MUTEX_INITIALIZER
  #define RTOS_LOCK_MUTEX(mutex)        pthread_mutex_lock(&mutex)
  #define RTOS_UNLOCK_MUTEX(mutex)      pthread_mutex_unlock(&mutex)
  #define RTOS_CREATE_FLAG(flag)        flag = 0  // TODO: real flags (use semaphores?)
  #define RTOS_SET_FLAG(flag)           flag = 1
  #define RTOS_CLEAR_FLAG(flag)         flag = 0

  static inline bool RTOS_WAIT_FLAG(RTOS_FLAG_HANDLE flag, uint32_t timeout)
  {
    simuSleep(timeout);
    return false;
  }

  #define RTOS_ISR_SET_FLAG RTOS_SET_FLAG

  template<int SIZE>
  class FakeTaskStack
  {
    public:
      FakeTaskStack()
      {
      }

      void paint()
      {
      }

      uint16_t size()
      {
        return SIZE;
      }

      uint16_t available()
      {
        return SIZE / 2;
      }
  };
  #define RTOS_DEFINE_STACK(name, size) FakeTaskStack<size> name
  #define RTOS_CREATE_TASK(taskId, task, name, stack, stackSize, priority) \
                                        pthread_create(&taskId, NULL, task, NULL);
  #define TASK_FUNCTION(task)           void * task(void * pdata)
  #define TASK_RETURN()                 return NULL

  constexpr uint16_t stackAvailable()
  {
    return 500;
  }

  // return 2ms resolution to match CoOS settings
  inline uint32_t RTOS_GET_TIME(void)
  {
    extern uint64_t simuTimerMicros(void);
    return simuTimerMicros() / 2000;
  }
    static inline uint32_t RTOS_GET_MS(void)
  {
    return (uint32_t)(simuTimerMicros() / 1000);
  }

  #define CoTickDelay(x) msleep(2*(x))  // TODO remove this later
#elif defined(RTOS_COOS)
  extern "C" {
    #include <CoOS.h>
  }
  #define RTOS_MS_PER_TICK              ((CFG_CPU_FREQ / CFG_SYSTICK_FREQ) / (CFG_CPU_FREQ / 1000))  // RTOS timer tick length in ms (currently 2)

  #define RTOS_INIT()                   CoInitOS()
  #define RTOS_WAIT_MS(x)               CoTickDelay((x)/2)
  #define RTOS_WAIT_TICKS(x)            CoTickDelay(x)
  #define RTOS_START()                  CoStartOS()
  #define RTOS_CREATE_TASK(taskId, task, name, stackStruct, stackSize, priority)   \
                                        taskId = CoCreateTask(task, NULL, priority, &stackStruct.stack[stackSize-1], stackSize)
  #define RTOS_TASK_HANDLE              OS_TID
  #define RTOS_MUTEX_HANDLE             OS_MutexID
  #define RTOS_FLAG_HANDLE              OS_FlagID
  #define RTOS_EVENT_HANDLE             OS_EventID
  #define RTOS_CREATE_MUTEX(mutex)      mutex = CoCreateMutex()
  #define RTOS_LOCK_MUTEX(mutex)        CoEnterMutexSection(mutex);
  #define RTOS_UNLOCK_MUTEX(mutex)      CoLeaveMutexSection(mutex);
  #define RTOS_CREATE_FLAG(flag)        flag = CoCreateFlag(false, false)
  #define RTOS_SET_FLAG(flag)           (void)CoSetFlag(flag)
  #define RTOS_CLEAR_FLAG(flag)         (void)CoClearFlag(flag)
   // RTOS timer tick length in ms (currently 2)
  #define RTOS_WAIT_FLAG(flag,timeout)  (CoWaitForSingleFlag(flag,timeout/2) == E_TIMEOUT)


  static inline void RTOS_ISR_SET_FLAG(RTOS_FLAG_HANDLE flag)
  {
    CoEnterISR();
    CoSchedLock();
    isr_SetFlag(flag);
    CoSchedUnlock();
    CoExitISR();
  }


  inline uint16_t getStackAvailable(void * address, uint16_t size)
  {
    uint32_t * array = (uint32_t *)address;
    uint16_t i = 0;
    while (i < size && array[i] == 0x55555555) {
      i++;
    }
    return i*4;
  }

  inline uint16_t stackSize()
  {
    return ((unsigned char *)&_estack - (unsigned char *)&_main_stack_start) / 4;
  }

  inline uint16_t stackAvailable()
  {
    return getStackAvailable(&_main_stack_start, stackSize());
  }

  template<int SIZE>
  class TaskStack
  {
    public:
      TaskStack()
      {
      }

      void paint()
      {
        for (uint32_t i=0; i<SIZE; i++) {
          stack[i] = 0x55555555;
        }
      }

      uint16_t size()
      {
        return SIZE * 4;
      }

      uint16_t available()
      {
        return getStackAvailable(stack, SIZE);
      }

      OS_STK stack[SIZE];
  };

  #define RTOS_DEFINE_STACK(name, size) TaskStack<size> __ALIGNED(8) name // stack must be aligned to 8 bytes otherwise printf for %f does not work!
  #define TASK_FUNCTION(task)           void task(void * pdata)
  #define TASK_RETURN()                 return
  inline uint32_t RTOS_GET_TIME(void)
  {
    return CoGetOSTime();
  }
  static inline uint32_t RTOS_GET_MS(void)
  {
    return (RTOS_GET_TIME() * RTOS_MS_PER_TICK);
  }
#else
  #define RTOS_WAIT_MS(x)               doNothing()
  #define RTOS_WAIT_TICKS(x)            doNothing()
  #define RTOS_START()                  doNothing()
#endif

#endif // _RTOS_H_
