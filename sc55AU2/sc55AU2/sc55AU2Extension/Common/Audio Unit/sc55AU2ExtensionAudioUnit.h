//
//  sc55AU2ExtensionAudioUnit.h
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

@interface sc55AU2ExtensionAudioUnit : AUAudioUnit
- (void)setupParameterTree:(AUParameterTree *)parameterTree;
@end
