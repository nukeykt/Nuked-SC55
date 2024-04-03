//
//  sc55AU2ExtensionAUProcessHelper.hpp
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#include <vector>
#include "sc55AU2ExtensionDSPKernel.hpp"

//MARK:- AUProcessHelper Utility Class
class AUProcessHelper
{
public:
    AUProcessHelper(sc55AU2ExtensionDSPKernel& kernel, UInt32 outputChannelCount)
    : mKernel{kernel},
    mOutputBuffers(outputChannelCount) {
    }
    
    /**
     This function handles the event list processing and rendering loop for you.
     Call it inside your internalRenderBlock.
     */
    void processWithEvents(AudioBufferList* outBufferList, AudioTimeStamp const *timestamp, AUAudioFrameCount frameCount, AURenderEvent const *events) {
        
        AUEventSampleTime now = AUEventSampleTime(timestamp->mSampleTime);
        AUAudioFrameCount framesRemaining = frameCount;
        AURenderEvent const *nextEvent = events; // events is a linked list, at the beginning, the nextEvent is the first event
        
        auto callProcess = [this] (AudioBufferList* outBufferListPtr, AUEventSampleTime now, AUAudioFrameCount frameCount, AUAudioFrameCount const frameOffset) {
            for (int channel = 0; channel < mOutputBuffers.size(); ++channel) {
                mOutputBuffers[channel] = (float*)outBufferListPtr->mBuffers[channel].mData + frameOffset;
            }
            
            mKernel.process(mOutputBuffers, now, frameCount, outBufferListPtr);
        };
        
        while (framesRemaining > 0) {
            // If there are no more events, we can process the entire remaining segment and exit.
            if (nextEvent == nullptr) {
                AUAudioFrameCount const frameOffset = frameCount - framesRemaining;
                callProcess(outBufferList, now, framesRemaining, frameOffset);
                return;
            }
            
            // // **** start late events late.
            // auto timeZero = AUEventSampleTime(0);
            // auto headEventTime = nextEvent->head.eventSampleTime;
            // AUAudioFrameCount framesThisSegment = AUAudioFrameCount(std::max(timeZero, headEventTime - now));
            
            // // Compute everything before the next event.
            // if (framesThisSegment > 0) {
            //     AUAudioFrameCount const frameOffset = frameCount - framesRemaining;
            //     callProcess(outBufferList, now, framesThisSegment, frameOffset);
                
            //     // Advance frames.
            //     framesRemaining -= framesThisSegment;
                
            //     // Advance time.
            //     now += AUEventSampleTime(framesThisSegment);
            // }
            
             nextEvent = performAllSimultaneousEvents(now, nextEvent);
        }
    }
    
     AURenderEvent const * performAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const *event) {
         do {
             mKernel.handleOneEvent(now, event);
            
             // Go to next event.
             event = event->head.next;
            
             // While event is not null and is simultaneous (or late).
         } while (event && event->head.eventSampleTime <= now);
         return event;
     }
private:
    sc55AU2ExtensionDSPKernel& mKernel;
    std::vector<float*> mOutputBuffers;
};
