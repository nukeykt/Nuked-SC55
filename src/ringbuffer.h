/*
 * Copyright (C) 2021, 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "math_util.h"

const int RB_CHANNEL_COUNT = 2;

struct ringbuffer_t {
    int16_t* samples;
    size_t frame_count;
    size_t sample_count;
    size_t read_head;
    size_t write_head;
    bool oversampling;
};

inline bool RB_Init(ringbuffer_t& rb, size_t frame_count)
{
    rb.samples = (int16_t*)calloc(RB_CHANNEL_COUNT * frame_count, sizeof(int16_t));
    if (!rb.samples)
    {
        return false;
    }
    rb.frame_count = frame_count;
    rb.sample_count = RB_CHANNEL_COUNT * frame_count;
    rb.read_head = 0;
    rb.write_head = 0;
    rb.oversampling = false;
    return true;
}

inline void RB_Free(ringbuffer_t& rb)
{
    free(rb.samples);
}

inline void RB_SetOversamplingEnabled(ringbuffer_t& rb, bool enabled)
{
    rb.oversampling = enabled;
    if (rb.oversampling)
    {
        rb.write_head &= ~3;
    }
    else
    {
        rb.write_head &= ~1;
    }
}

inline bool RB_IsFull(ringbuffer_t& rb)
{
    if (rb.oversampling)
    {
        return ((rb.write_head + 2 * RB_CHANNEL_COUNT) % rb.sample_count) == rb.read_head;
    }
    else
    {
        return ((rb.write_head + 1 * RB_CHANNEL_COUNT) % rb.sample_count) == rb.read_head;
    }
}

inline void RB_Write(ringbuffer_t& rb, int16_t left, int16_t right)
{
    rb.samples[rb.write_head + 0] = left;
    rb.samples[rb.write_head + 1] = right;
    rb.write_head = (rb.write_head + RB_CHANNEL_COUNT) % rb.sample_count;
}

inline size_t RB_ReadableSampleCount(ringbuffer_t& rb)
{
    if (rb.read_head <= rb.write_head)
    {
        return rb.write_head - rb.read_head;
    }
    else
    {
        return rb.sample_count - (rb.read_head - rb.write_head);
    }
}

// Reads up to `sample_count` samples and returns the number of samples
// actually read.
inline size_t RB_Read(ringbuffer_t& rb, int16_t* dest, size_t sample_count)
{
    size_t have_count = RB_ReadableSampleCount(rb);
    size_t read_count = sample_count < have_count ? sample_count : have_count;
    size_t read_head = rb.read_head;
    // TODO make this one or two memcpys
    for (size_t i = 0; i < read_count; ++i)
    {
        *dest = rb.samples[read_head];
        ++dest;
        read_head = (read_head + 1) % rb.sample_count;
    }
    rb.read_head = read_head;
    return read_count;
}

// Reads up to `sample_count` samples and returns the number of samples
// actually read. Mixes samples into dest by adding and clipping.
inline size_t RB_ReadMix(ringbuffer_t& rb, int16_t* dest, size_t sample_count)
{
    size_t have_count = RB_ReadableSampleCount(rb);
    size_t read_count = sample_count < have_count ? sample_count : have_count;
    size_t read_head = rb.read_head;
    for (size_t i = 0; i < read_count; ++i)
    {
        *dest = saturating_add(*dest, rb.samples[read_head]);
        ++dest;
        read_head = (read_head + 1) % rb.sample_count;
    }
    rb.read_head = read_head;
    return read_count;
}

