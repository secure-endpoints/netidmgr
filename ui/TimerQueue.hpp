/*
 * Copyright (c) 2009 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

namespace nim {

    class NOINITVTABLE TimerQueueClient {
        friend class TimerQueueHost;

    private:
        HANDLE timer;
        HANDLE timerQueue;

    public:
        TimerQueueClient() {
            timer = NULL;
            timerQueue = NULL;
        }

        virtual ~TimerQueueClient() {
            CancelTimer();
        }

        virtual void OnTimer() {
            // This can't be a pure virtual function.  The timer queue
            // may run in a separate thread, in which case the timer
            // may fire after a derived class destructor has run, but
            // before the destructor for this class has run.  In this
            // case, the derived class vtable would no longer exist
            // and the callback has to be received by this function.
        }

        void CancelTimer() {
            HANDLE p_timer;
#pragma warning(push)
#pragma warning(disable: 4312)
            p_timer = InterlockedExchangePointer(&timer, NULL);
#pragma warning(pop)
            if (p_timer != NULL && timerQueue != NULL) {
                DeleteTimerQueueTimer(timerQueue, p_timer,
                                      INVALID_HANDLE_VALUE);
                timerQueue = NULL;
            }
        }
    };

    class NOINITVTABLE TimerQueueHost {
    protected:
        HANDLE timerQueue;

        static VOID CALLBACK TimerCallback(PVOID param, BOOLEAN waitOrTimer) {
            TimerQueueClient * cb = static_cast<TimerQueueClient *>(param);
            if (cb->timer)
                cb->OnTimer();
        }

    public:
        TimerQueueHost() { 
            timerQueue = CreateTimerQueue();
        }

        virtual ~TimerQueueHost() {
            if (timerQueue) {
                DeleteTimerQueueEx(timerQueue, INVALID_HANDLE_VALUE);
            }
        }

        void SetTimer(TimerQueueClient * cb, DWORD milliseconds) {
            HANDLE timer;

#ifdef DEBUG
            kherr_debug_printf(L"SetTimer(%S, %d ms)\n",
                               typeid(*cb).name(), milliseconds);
#endif
            if (CreateTimerQueueTimer(&timer, timerQueue, TimerCallback,
                                      cb, milliseconds, 0, WT_EXECUTEONLYONCE)) {
                cb->timer = timer;
                cb->timerQueue = timerQueue;
            }
        }

        void SetTimerRepeat(TimerQueueClient * cb, DWORD ms_start, DWORD ms_repeat) {
            HANDLE timer;
            if (CreateTimerQueueTimer(&timer, timerQueue, TimerCallback,
                                      cb, ms_start, ms_repeat, 0)) {
                cb->timer = timer;
                cb->timerQueue = timerQueue;
            }
        }
    };
}
