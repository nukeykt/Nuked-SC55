//
//  ObservableAUParameter.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

import SwiftUI
import AudioToolbox

/// Base-class for SwiftUI-capable AUParameterNodes
///
/// This implementation provides a central point AUParameterGroup nodes to build a set of
/// observable children, and also enables us to traverse the parameter tree using dynamicMemberLookup
/// and subscript notation (i.e. parameterTree.paramGroup.parameter)
///
/// This does *not* provide any of Swift's usual type-safety benefits, and may result in fatal errors if the
/// implementation attempts to access the subscript of an ObservableAUParameter (which has no children, as it's not a group).
@dynamicMemberLookup
class ObservableAUParameterNode {

    /// Create an ObservableAUParameterNode
    ///
    /// This creates the appropriate subclass, depending on the type of the passed in AUParameterNode
    class func create(_ parameterNode: AUParameterNode) -> ObservableAUParameterNode {
        switch parameterNode {
        case let parameter as AUParameter:
            return ObservableAUParameter(parameter)
        case let group as AUParameterGroup:
            return ObservableAUParameterGroup(group)
        default:
            fatalError("Unexpected AUParameterNode subclass")
        }
    }

    subscript<T>(dynamicMember identifier: String) -> T {
        guard let groupSelf = self as? ObservableAUParameterGroup else {
            fatalError("Calling subscript is only supported on ObservableAUParameterGroups, you called it on \(self)")
        }

        guard let node = groupSelf.children[identifier] else {
            if groupSelf.children.isEmpty {
                fatalError("This group has no children")
            }

            let availableChildren = groupSelf.children.keys.joined(separator: "\n")

            print("Parameter Group \(groupSelf) doesn't have a child node named \(identifier), did you mean one of: \n \(availableChildren)")
            fatalError()
        }

        guard let subNode = node as? T else {
            fatalError("Parameter node named \(identifier) cannot be converted to the requested type")
        }

        return subNode
    }

    subscript(dynamicMember identifier: String) -> ObservableAUParameterNode {
        guard let groupSelf = self as? ObservableAUParameterGroup else {
            fatalError("Calling subscript is only supported on ObservableAUParameterGroups, you called it on \(self)")
        }

        guard let parameter = groupSelf.children[identifier] else {
            if groupSelf.children.isEmpty {
                fatalError("This group has no children")
            }

            let availableChildren = groupSelf.children.keys.joined(separator: "\n")

            print("Parameter Group \(groupSelf) doesn't have a child node named \(identifier), did you mean one of: \n \(availableChildren)")
            fatalError()
        }

        return parameter
    }

    private func asParameter() -> ObservableAUParameter {
        guard let parameter = self as? ObservableAUParameter else {
            fatalError("Node is not a parameter")
        }
        return parameter
    }

    subscript(dynamicMember keyPath: ReferenceWritableKeyPath<ObservableAUParameter, Float>) -> Float {
        get { self.asParameter()[keyPath: keyPath] }
        set { self.asParameter()[keyPath: keyPath] = newValue }
    }
}

/// An Observable version of AUParameterGroup
///
/// The primary purpose here is to expose observable versions of the group's child parameters.
///
final class ObservableAUParameterGroup: ObservableAUParameterNode {

    private(set) var children: [String: ObservableAUParameterNode]

    init(_ parameterGroup: AUParameterGroup) {
        children = parameterGroup.children.reduce(
            into: [String: ObservableAUParameterNode]()
        ) { dict, node in
            let observableNode = ObservableAUParameterNode.create(node)
            dict[node.identifier] = observableNode
        }
    }
}

/// An Observable version of AUParameter
///
/// ObservableAUParameter is intended to be used directly in SwiftUI views as an ObservedObject,
/// allowing us to expose a binding to the parameter's value, as well as associated parameter data,
/// like the minimum, maximum, and default values for the parameter.
///
/// The ObservableAUParameter can also manage automation event types by calling
/// `onEditingChanged()` whenever a UI element will change its editing state.
final class ObservableAUParameter: ObservableAUParameterNode, ObservableObject {

    private weak var parameter: AUParameter?
    private var observerToken: AUParameterObserverToken!
    private var editingState: EditingState = .inactive

    let min: AUValue
    let max: AUValue
    let displayName: String
    let defaultValue: AUValue = 0.0
    let unit: AudioUnitParameterUnit

    init(_ parameter: AUParameter) {
        self.parameter = parameter
        self.value = parameter.value
        self.min = parameter.minValue
        self.max = parameter.maxValue
        self.displayName = parameter.displayName
        self.unit = parameter.unit
        super.init()

        /// Use the parameter.token(byAddingParameterObserver:) function to monitor for parameter
        /// changes from the host. The only role of this callback is to update the UI if the value is changed by the host.
        self.observerToken = parameter.token { (_ address: AUParameterAddress, _ auValue: AUValue) in
            guard address == self.parameter?.address else { return }

            DispatchQueue.main.async {
                // Don't update the UI if the user is currently interacting
                guard self.editingState == .inactive else { return }

                self.editingState = .hostUpdate
                self.value = auValue
                self.editingState = .inactive
            }
        }
    }

    @Published var value: AUValue {
        didSet {
            /// If the editing state is .hostUpdate, don't propagate this back to the host
            guard editingState != .hostUpdate else { return }

            let automationEventType = resolveEventType()
            parameter?.setValue(
                value,
                originator: observerToken,
                atHostTime: 0,
                eventType: automationEventType
            )
            print("Param was set \(value)")
        }
    }

    /// A callback for UI elements to notify the Parameter when UI editing state changes
    ///
    /// This is the core mechanism for ensuring correct automation behavior. With native SwiftUI elements like `Slider`,
    /// this method should be passed directly into the `onEditingChanged:` argument.
    ///
    /// As long as the UI Element correctly sets the editing state, then the ObservableAUParameter's calls to
    /// AUParameter.setValue will contain the correct automation event type.
    ///
    /// `onEditingChanged` should be called with `true` before the first value is sent, so that it can be sent with a
    /// `.touch` event. It's expected that `onEditingChanged` is called with a value of `false` to mark the end
    /// of interaction *after* the last value has been sent, since this is how SwiftUI's `Slider` and `Stepper` views behave.
    func onEditingChanged(_ editing: Bool) {
        if editing {
            editingState = .began
        } else {
            editingState = .ended

            // We set the value here again to prompt its `didSet` implementation, so that we can send the appropriate `.release` event.
            value = value
        }
    }

    private func resolveEventType() -> AUParameterAutomationEventType {
        let eventType: AUParameterAutomationEventType
        switch editingState {
        case .began:
            eventType = .touch
            editingState = .active
        case .ended:
            eventType = .release
            editingState = .inactive
        default:
            eventType = .value
        }
        return eventType
    }

    private enum EditingState {
        case inactive
        case began
        case active
        case ended
        case hostUpdate
    }
}

extension AUAudioUnit {
    // Can we subclass the Parameter tree to set that on the AUAudioUnit?

    var observableParameterTree: ObservableAUParameterGroup? {
        guard let paramTree = self.parameterTree else { return nil }
        return ObservableAUParameterGroup(paramTree)
    }
}
