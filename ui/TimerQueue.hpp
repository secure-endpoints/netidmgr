/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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
    friend class DisplayContainer;

 private:
    UINT_PTR m_timer_id;

    typedef void (*CancelTimerCallback)(TimerQueueClient *, void *);

    CancelTimerCallback m_timer_ccb;
    void * m_timer_ccb_data;

 public:
    TimerQueueClient() {
        m_timer_id = 0;
        m_timer_ccb = NULL;
        m_timer_ccb_data = NULL;
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
        if (m_timer_ccb) {
            (*m_timer_ccb)(this, m_timer_ccb_data);
        }
    }
};
}
