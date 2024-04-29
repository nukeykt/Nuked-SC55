#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

const int RB_CHANNEL_COUNT = 2;

struct ringbuffer_t {
    int16_t* samples;
    size_t frame_count;
    size_t sample_count;
    size_t read_head;
    size_t write_head;
    bool oversampling;
};

inline void RB_Init(ringbuffer_t& rb, size_t frame_count)
{
    rb.samples = (int16_t*)malloc(sizeof(int16_t) * RB_CHANNEL_COUNT * frame_count);
    memset(rb.samples, 0, sizeof(int16_t) * RB_CHANNEL_COUNT * frame_count);
    rb.frame_count = frame_count;
    rb.sample_count = RB_CHANNEL_COUNT * frame_count;
    rb.read_head = 0;
    rb.write_head = 0;
    rb.oversampling = false;
}

inline void RB_Free(ringbuffer_t& rb)
{
    free(rb.samples);
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
    if (RB_IsFull(rb))
    {
        fprintf(stderr, "Ringbuffer overflow\n");
        exit(1);
    }
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
// actually read
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

