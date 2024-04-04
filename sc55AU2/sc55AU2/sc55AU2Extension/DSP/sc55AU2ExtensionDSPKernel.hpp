//
//  sc55AU2ExtensionDSPKernel.hpp
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <CoreMIDI/CoreMIDI.h>
//#include <chrono>

#import "sc55AU2Extension-Swift.h"
#import "sc55AU2ExtensionParameterAddresses.h"
#import <mcu.h>

OSStatus EncoderDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets,
                AudioBufferList *ioData,
                AudioStreamPacketDescription **outDataPacketDescription,
                void *inUserData);

/*
 sc55AU2ExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class sc55AU2ExtensionDSPKernel {
public:
    void initialize(int channelCount, double inSampleRate, const AudioStreamBasicDescription *outputFormat) {
        destFormat = *outputFormat;

        AudioStreamBasicDescription sourceDescription;
        sourceDescription.mSampleRate = 66207;
        sourceDescription.mBytesPerFrame = 4;
        sourceDescription.mBitsPerChannel = 16;
        sourceDescription.mFormatID = kAudioFormatLinearPCM;
        sourceDescription.mBytesPerPacket = 4;
        sourceDescription.mChannelsPerFrame = 2;
        sourceDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger;
        sourceDescription.mFramesPerPacket = 1;
        sourceDescription.mReserved = 0;

        AudioConverterNew(&sourceDescription, &destFormat, &audioConverterRef);

        auto path = std::string(NSBundle.mainBundle.bundleURL.absoluteString.UTF8String).substr(7) + "Contents/Resources";
        mcu.startSC55(&path);
    }
    
    void deInitialize() {
    }
    
    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }
    
    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }
    
    // MARK: - Parameter Getter / Setter
    // Add a case for each parameter in sc55AU2ExtensionParameterAddresses.h
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case sc55AU2ExtensionParameterAddress::gain:
                break;
        }
    }
    
    AUValue getParameter(AUParameterAddress address) {
        // Return the goal. It is not thread safe to return the ramping value.
        
        switch (address) {
            case sc55AU2ExtensionParameterAddress::gain:
                return (AUValue)0;
                
            default: return 0.f;
        }
    }
    
    // MARK: - Max Frames
    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }
    
    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }
    
    // MARK: - Musical Context
    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }
    
    // MARK: - MIDI Protocol
    MIDIProtocolID AudioUnitMIDIProtocol() const {
        return kMIDIProtocol_1_0;
    }
    
    /**
     MARK: - Internal Process
     
     This function does the core siginal processing.
     Do your custom DSP here.
     */
    void process(AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount, AudioBufferList* outBufferList) {
        // auto start = std::chrono::high_resolution_clock::now();

        UInt32 ioOutputDataPackets = frameCount * destFormat.mFramesPerPacket;
        AudioConverterFillComplexBuffer(audioConverterRef, EncoderDataProc,
                                        (void *)this, &ioOutputDataPackets,
                                        outBufferList, NULL);


        // auto stop = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        // if (duration > 8) {
        //     printf("process took %lld ms\n", duration);
        //     fflush(stdout);
        // }
    }
    
    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
                
            case AURenderEventMIDIEventList: {
                handleMIDIEventList(now, &event->MIDIEventsList);
                break;
            }
                
            default:
                break;
        }
    }
    
    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        // Implement handling incoming Parameter events as needed
    }
    
    void handleMIDIEventList(AUEventSampleTime now, AUMIDIEventList const* midiEvent) {
        auto visitor = [] (void* context, MIDITimeStamp timeStamp, MIDIUniversalMessage message) {
            auto thisObject = static_cast<sc55AU2ExtensionDSPKernel *>(context);
            
            switch (message.type) {
                case kMIDIMessageTypeChannelVoice1: {
                    thisObject->handleMIDI1VoiceMessage(message);
                    break;
                }
                    
                default:
                    break;
            }
        };
        
        MIDIEventListForEachEvent(&midiEvent->eventList, visitor, this);
    }
    
    void handleMIDI1VoiceMessage(const struct MIDIUniversalMessage& message) {
        uint8_t messageBytes[8];
        int length = message.channelVoice1.status == kMIDICVStatusNoteOn
                  || message.channelVoice1.status == kMIDICVStatusNoteOff
                  || message.channelVoice1.status == kMIDICVStatusPolyPressure
                  || message.channelVoice1.status == kMIDICVStatusControlChange
                  || message.channelVoice1.status == kMIDICVStatusPitchBend ? 3 : 2;
        messageBytes[0] = message.channelVoice1.status << 4 | message.channelVoice1.channel;
        if (message.channelVoice1.status == kMIDICVStatusPitchBend) {
            messageBytes[1] = message.channelVoice1.pitchBend & 0x7F;
            messageBytes[2] = (message.channelVoice1.pitchBend >> 7) & 0x7F;
        } else {
            messageBytes[1] = message.channelVoice1.note.number;
            messageBytes[2] = message.channelVoice1.note.velocity;
        }
        mcu.postMidiSC55(messageBytes, length);
    }
    
    // MARK: - Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;
    
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    AudioConverterRef audioConverterRef;
    AudioStreamBasicDescription destFormat;

    int16_t *lastBufferData;
    
public:
    MCU mcu;
};

OSStatus EncoderDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets,
                AudioBufferList *ioData,
                AudioStreamPacketDescription **outDataPacketDescription,
                void *inUserData) {
  sc55AU2ExtensionDSPKernel *_this = (sc55AU2ExtensionDSPKernel *)inUserData;

  if (_this->lastBufferData) {
    free(_this->lastBufferData);
  }

  unsigned int amountToWrite = *ioNumberDataPackets;

  unsigned int dataSize = amountToWrite * 2 * 2;
  int16_t *dataBuff = (int16_t *)malloc(dataSize);
//   memset(dataBuff, 0, dataSize);
  _this->mcu.updateSC55(dataBuff, dataSize);
  _this->lastBufferData = dataBuff;

  ioData->mNumberBuffers = 1;
  ioData->mBuffers[0].mData = dataBuff;
  ioData->mBuffers[0].mDataByteSize = dataSize;

  return 0;
}
