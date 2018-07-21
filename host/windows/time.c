// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/types.h>
#include <openenclave/internal/time.h>
#include <windows.h>

/* Return the microseconds elapsed since the Epoch. */
static uint64_t _time()
{
    FILETIME ft;
    ULARGE_INTEGER x;

    GetSystemTimeAsFileTime(&ft);
    x.u.LowPart = ft.dwLowDateTime;
    x.u.HighPart = ft.dwHighDateTime;
    x.QuadPart -= 0X19DB1DED53E8000;

    return = x.QuadPart / 10UL;
}

void oe_handle_sleep_ocall(uint64_t arg_in)
{
    const uint64_t milliseconds = arg_in;
    Sleep(milliseconds);
}

void oe_handle_untrusted_time_ocall(uint64_t arg_in, uint64_t* arg_out)
{
    OE_UNUSED(arg_in);

    if (arg_out)
        *arg_out = _time();
}
