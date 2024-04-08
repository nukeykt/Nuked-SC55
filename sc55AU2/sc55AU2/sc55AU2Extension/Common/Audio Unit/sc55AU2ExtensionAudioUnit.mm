//
//  sc55AU2ExtensionAudioUnit.mm
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#import "sc55AU2ExtensionAudioUnit.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/AUViewController.h>

#import "sc55AU2ExtensionAUProcessHelper.hpp"
#import "sc55AU2ExtensionDSPKernel.hpp"


// Define parameter addresses.

@interface sc55AU2ExtensionAudioUnit ()

@property (nonatomic, readwrite) AUParameterTree *parameterTree;
@property AUAudioUnitBusArray *outputBusArray;
@property (nonatomic, readonly) AUAudioUnitBus *outputBus;
@end


@implementation sc55AU2ExtensionAudioUnit {
    // C++ members need to be ivars; they would be copied on access if they were properties.
    sc55AU2ExtensionDSPKernel _kernel;
    std::unique_ptr<AUProcessHelper> _processHelper;
}

@synthesize parameterTree = _parameterTree;

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription options:(AudioComponentInstantiationOptions)options error:(NSError **)outError {
    self = [super initWithComponentDescription:componentDescription options:options error:outError];
    
    if (self == nil) { return nil; }
    
    [self setupAudioBuses];
    
    return self;
}

#pragma mark - AUAudioUnit Setup

- (void)setupAudioBuses {
    // Create the output bus first
    AVAudioFormat *format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
    _outputBus = [[AUAudioUnitBus alloc] initWithFormat:format error:nil];
    _outputBus.maximumChannelCount = 8;
    
    // then an array with it
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                             busType:AUAudioUnitBusTypeOutput
                                                              busses: @[_outputBus]];
}

- (void)setupParameterTree:(AUParameterTree *)parameterTree {
    _parameterTree = parameterTree;
    
    // Send the Parameter default values to the Kernel before setting up the parameter callbacks, so that the defaults set in the Kernel.hpp don't propagate back to the AUParameters via GetParameter
    for (AUParameter *param in _parameterTree.allParameters) {
        _kernel.setParameter(param.address, param.value);
    }
    
    [self setupParameterCallbacks];
}

- (void)setupParameterCallbacks {
    // Make a local pointer to the kernel to avoid capturing self.
    
    __block sc55AU2ExtensionDSPKernel *kernel = &_kernel;
    
    // implementorValueObserver is called when a parameter changes value.
    _parameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
        kernel->setParameter(param.address, value);
    };
    
    // implementorValueProvider is called when the value needs to be refreshed.
    _parameterTree.implementorValueProvider = ^(AUParameter *param) {
        return kernel->getParameter(param.address);
    };
    
    // A function to provide string representations of parameter values.
    _parameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
        AUValue value = valuePtr == nil ? param.value : *valuePtr;
        
        return [NSString stringWithFormat:@"%.f", value];
    };
}

#pragma mark - AUAudioUnit Overrides

- (AUAudioFrameCount)maximumFramesToRender {
    return _kernel.maximumFramesToRender();
}

- (void)setMaximumFramesToRender:(AUAudioFrameCount)maximumFramesToRender {
    _kernel.setMaximumFramesToRender(maximumFramesToRender);
}

// An audio unit's audio output connection points.
// Subclassers must override this property getter and should return the same object every time.
// See sample code.
- (AUAudioUnitBusArray *)outputBusses {
    return _outputBusArray;
}

- (void)setShouldBypassEffect:(BOOL)shouldBypassEffect {
    _kernel.setBypass(shouldBypassEffect);
}

- (BOOL)shouldBypassEffect {
    return _kernel.isBypassed();
}

// Allocate resources required to render.
// Subclassers should call the superclass implementation.
- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError {
    const auto outputChannelCount = [self.outputBusses objectAtIndexedSubscript:0].format.channelCount;
    const auto outputFormat = [self.outputBusses objectAtIndexedSubscript:0].format.streamDescription;
    
    _kernel.setMusicalContextBlock(self.musicalContextBlock);
    _kernel.initialize(outputChannelCount, _outputBus.format.sampleRate, outputFormat);
    _processHelper = std::make_unique<AUProcessHelper>(_kernel, outputChannelCount);
    return [super allocateRenderResourcesAndReturnError:outError];
}

// Deallocate resources allocated in allocateRenderResourcesAndReturnError:
// Subclassers should call the superclass implementation.
- (void)deallocateRenderResources {
    
    // Deallocate your resources.
    _kernel.deInitialize();
    
    [super deallocateRenderResources];
}

#pragma mark - MIDI

- (MIDIProtocolID)AudioUnitMIDIProtocol {
    return _kernel.AudioUnitMIDIProtocol();
}

#pragma mark - AUAudioUnit (AUAudioUnitImplementation)

// Block which subclassers must provide to implement rendering.
- (AUInternalRenderBlock)internalRenderBlock {
    /*
     Capture in locals to avoid ObjC member lookups. If "self" is captured in
     render, we're doing it wrong.
     */
    // Specify captured objects are mutable.
    __block sc55AU2ExtensionDSPKernel *kernel = &_kernel;
    __block std::unique_ptr<AUProcessHelper> &processHelper = _processHelper;
    
    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags 				*actionFlags,
                              const AudioTimeStamp       				*timestamp,
                              AVAudioFrameCount           				frameCount,
                              NSInteger                   				outputBusNumber,
                              AudioBufferList            				*outputData,
                              const AURenderEvent        				*realtimeEventListHead,
                              AURenderPullInputBlock __unsafe_unretained pullInputBlock) {
        
        if (frameCount > kernel->maximumFramesToRender()) {
            return kAudioUnitErr_TooManyFramesToProcess;
        }
        
        /*
         Important:
         If the caller passed non-null output pointers (outputData->mBuffers[x].mData), use those.
         
         If the caller passed null output buffer pointers, process in memory owned by the Audio Unit
         and modify the (outputData->mBuffers[x].mData) pointers to point to this owned memory.
         The Audio Unit is responsible for preserving the validity of this memory until the next call to render,
         or deallocateRenderResources is called.
         
         If your algorithm cannot process in-place, you will need to preallocate an output buffer
         and use it here.
         
         See the description of the canProcessInPlace property.
         */
        processHelper->processWithEvents(outputData, timestamp, frameCount, realtimeEventListHead);
        
        return noErr;
    };
}

- (void)SC55_Reset {
    _kernel.mcu->SC55_Reset();
};

- (uint32_t*)LCD_Update {
    return _kernel.mcu->lcd.LCD_Update();
};

- (void)LCD_SendButton:(uint8_t) button: (int) state {
    _kernel.mcu->lcd.LCD_SendButton(button, state);
};

@end

