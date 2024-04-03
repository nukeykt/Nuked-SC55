# Audio Unit Extension
This template serves as a starting point to create a custom plug-in using the latest Audio Unit standard (AUv3). The AUv3 standard builds on the App Extensions model, which means you deliver your plug-in as an extension that’s contained in an app distributed through the App Store or your own store.

There are 5 types of Audio Unit Extensions, each type is represented by a four character code.

|Name|Four Character Code|
|---|---|
|Effect|aufx|
|Music Effect|aumf|
|MIDI Processor|aumi|
|Instrument|aumu|
|Generator|augn|


## Languages
This template uses Swift and SwiftUI for business logic and user interface, C++ for real-time constrained areas and Objective-C for interacting between Swift and C++.

## Project Layout
This template is designed to make Audio Unit development as easy as possible. In most cases you should only need to edit files in the top level groups; `Parameters`, `DSP` and `UI` groups.

* /Common - Contains common code split by functionality which should rarely need to be modified. 
	* `Audio Unit/sc55AU2ExtensionAudioUnit.mm/h` - A subclass of AUAudioUnit, this is the actual Audio Unit implementation. You may in advanced cases need to change this file to add additional functionality from AUAudioUnit.  
* /Parameters
	* `sc55AU2ExtensionParameterAddresses.h` - A pure `C` enum containing parameter addresses used by Swift and C++ to reference parameters.
	
	* `Parameters.swift` - Contains a ParameterTreeSpec object made up of ParameterGroupSpec's and ParameterSpec's which allow you describe your plug-in's parameters and the layout of those parameters.

* /DSP
	* `sc55AU2ExtensionDSPKernel.hpp` - A pure C++ class to handle the real-time aspects of the Audio Unit Extension. DSP and processing should be done here. Note: Be aware of the constraints of real-time audio processing. 
* /UI
	* `sc55AU2ExtensionMainView.swift` - SwiftUI based main view, add your SwiftUI views and controls here.

## Adding a parameter
1. Add a new parameter address to the `sc55AU2ExtensionParameterAddress` enum in `sc55AU2ExtensionParameterAddresses.h` 


Example:

```c
typedef NS_ENUM(AUParameterAddress, sc55AU2ExtensionParameterAddress) {
	sendNote = 0,
	....
	attack
```

2. Create a new `ParameterSpec` in `Parameters.swift` using the enum value (created in step 1) as the address.

Example:

```swift
ParameterGroupSpec(identifier: "global", name: "Global") {
	....
	ParameterSpec(
		address: .attack,
		identifier: "attack",
		name: "Attack",
		units: .milliseconds,
		valueRange: 0.0...1000.0,
		defaultValue: 100.0
	)
	...
```
Note: the identifier will be used to interact with this parameter from SwiftUI.

3. In order to manipulate the DSP side of the Audio Unit we must handle changes to our new parameter in `sc55AU2ExtensionDSPKernel.hpp`. In the `setParameter` and `getParameter` methods add a case for the new parameter address.

Example:

```cpp
	void setParameter(AUParameterAddress address, AUValue value) {
		switch (address) {
			....
			case sc55AU2ExtensionExtensionParameterAddress:: attack:
				mAttack = value;
				break;			
			...
	}
	
	AUValue getParameter(AUParameterAddress address) {
		switch (address) {
			....
			case sc55AU2ExtensionExtensionParameterAddress::attack:
				return (AUValue) mAttack;
			...
	}
	
	// You can now apply attack your DSP algorithm using `mAttack` in the `process` call. 
```

4. For Audio Units that present a user interface, you should expose or access the new parameter in your SwiftUI view. The parameter can be accessed using its identifier (defined in step 2). It is accessed using dot notation as follows parameterTree.<ParameterGroupSpec Identifier>.<ParameterGroupSpec Identifier>.<ParameterSpec Identifier>

Example

```Swift
// Access the attack parameters value from SwiftUI
parameterTree.global.attack.value

// Set the attack parameters value from SwiftUI
parameterTree.global.attack.value = 0.5

// Bind the parameter to a slider
struct EqualizerExtensionMainView: View {
	...	
	var body: some View {
		ParameterSlider(param: parameterTree.global.attack)
	}
	...
}

/*
Note: the parameterTree.<parameter_name> must match the structure and identifier of the parameter defined in `Parameters.swift`.
*/
```

## Catalyst / iPhone and iPad apps on Mac with Apple silicon
To build this template in a Catalyst or iPhone/iPad App on Mac with Apple silicon, perform the following steps:  

1. Select your Xcode project in the left hand side file browser
2. Select your app target under the 'TARGETS' menu
3. Under 'Deployment Info' select 'Mac Catalyst' (Note: Skip this step for iPhone and iPad apps on Mac with Apple silicon)
4. Select the 'General' tab in the top menu bar
5. Under 'Frameworks, Libraries, and Embedded Content' click the button next to the  iOS filter
6. In the pop-up menu select 'Allow any platforms'

## More Information
[Apple Audio Developer Documentation](https://developer.apple.com/audio/)
