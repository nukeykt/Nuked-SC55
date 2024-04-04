//
//  TypeAliases.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import CoreMIDI
import AudioToolbox

#if os(iOS)
import UIKit
public typealias KitColor = UIColor

public typealias KitView = UIView
public typealias ViewController = UIViewController
#elseif os(macOS)
import AppKit
public typealias KitColor = NSColor

public typealias KitView = NSView
public typealias ViewController = NSViewController
#endif
