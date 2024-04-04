//
//  sc55AU2ExtensionParameterAddresses.h
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

#ifdef __cplusplus
namespace sc55AU2ExtensionParameterAddress {
#endif

typedef NS_ENUM(AUParameterAddress, sc55AU2ExtensionParameterAddress) {
    gain = 0
};

#ifdef __cplusplus
}
#endif
