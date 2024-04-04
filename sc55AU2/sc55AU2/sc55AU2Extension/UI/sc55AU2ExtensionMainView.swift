//
//  sc55AU2ExtensionMainView.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

import SwiftUI

public struct Color {
    public var r, g, b, a: UInt8
    
    public init(r: UInt8, g: UInt8, b: UInt8, a: UInt8 = 255) {
        self.r = r
        self.g = g
        self.b = b
        self.a = a
    }
}

extension Image {
    init?(bitmap: Bitmap) {
        let alphaInfo = CGImageAlphaInfo.premultipliedLast
        let bytesPerPixel = MemoryLayout<Color>.size
        let bytesPerRow = bitmap.width * bytesPerPixel

        guard let providerRef = CGDataProvider(data: Data(
            bytes: bitmap.pixels, count: bitmap.height * bytesPerRow
        ) as CFData) else {
            return nil
        }

        guard let cgImage = CGImage(
            width: bitmap.width,
            height: bitmap.height,
            bitsPerComponent: 8,
            bitsPerPixel: bytesPerPixel * 8,
            bytesPerRow: bytesPerRow,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGBitmapInfo(rawValue: alphaInfo.rawValue),
            provider: providerRef,
            decode: nil,
            shouldInterpolate: true,
            intent: .defaultIntent
        ) else {
            return nil
        }

        self.init(decorative: cgImage, scale: 1.0, orientation: .up)
    }
}

public struct Bitmap {
    public private(set) var pixels: [Color]
    public let width: Int
    
    public init(width: Int, pixels: [Color]) {
        self.width  = width
        self.pixels = pixels
    }
    
    var height: Int {
        return pixels.count / width
    }
    
    subscript(x: Int, y: Int) -> Color {
        get { return pixels[y * width + x] }
        set { pixels[y * width + x] = newValue }
    }

    init(width: Int, height: Int, color: Color) {
        self.pixels = Array(repeating: color, count: width * height)
        self.width  = width
    }
}

public struct Renderer {
    public private(set) var bitmap: Bitmap
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    public init(width: Int, height: Int) {
        self.bitmap = Bitmap(width: width, height: height, color: Color.init(
            r: 0, g: 0,b: 0, a: 0
        ))
    }
    
    mutating func draw() {
        if let audioUnitR = audioUnit {
            let lcdResult = audioUnitR.lcd_Update();
             for x in 0...(741-1) {
                 for y in 0...(268-1) {
                     bitmap[x, y] = Color.init(
                         r: UInt8((lcdResult![x + y * 741] >> 0) & 0xff),
                         g: UInt8((lcdResult![x + y * 741] >> 8) & 0xff),
                         b: UInt8((lcdResult![x + y * 741] >> 16) & 0xff),
                         a: 255
                     )
                 }
             }
        }
    }
}

struct SCButton: View {
    let code: Int
    let text: String
    var audioUnit: sc55AU2ExtensionAudioUnit
    
    var body: some View {
        Button(action: {
            audioUnit.lcd_SendButton(UInt8(code), 1)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                audioUnit.lcd_SendButton(UInt8(code), 0)
            }
        }){
            Text(text)
        }
    }
}

struct sc55AU2ExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup
    var audioUnit: sc55AU2ExtensionAudioUnit
    
    @ObservedObject
    var screen = Screen(width: 741, height: 268)
    
    var body: some View {
        screen.image?
            .resizable()
            .interpolation(.none)
            .frame(width: 741, height: 268, alignment: .center)
            .aspectRatio(contentMode: .fit)
            .onAppear(perform: { screen.renderer.audioUnit = audioUnit })
        
        VStack {
            HStack {
                SCButton(code: MCU_BUTTON_POWER, text: "POWER", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_INST_L, text: "INST_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_INST_R, text: "INST_R", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_INST_MUTE, text: "INST_MUTE", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_INST_ALL, text: "INST_ALL", audioUnit: audioUnit)
            }
            HStack {
                SCButton(code: MCU_BUTTON_MIDI_CH_L, text: "MIDI_CH_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_MIDI_CH_R, text: "MIDI_CH_R", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_CHORUS_L, text: "CHORUS_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_CHORUS_R, text: "CHORUS_R", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_PAN_L, text: "PAN_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_PAN_R, text: "PAN_R", audioUnit: audioUnit)
            }
            HStack {
                SCButton(code: MCU_BUTTON_PART_L, text: "PART_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_PART_R, text: "PART_R", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_KEY_SHIFT_L, text: "KEY_SHIFT_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_KEY_SHIFT_R, text: "KEY_SHIFT_R", audioUnit: audioUnit)
            }
            HStack {
                SCButton(code: MCU_BUTTON_LEVEL_L, text: "LEVEL_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_LEVEL_R, text: "LEVEL_R", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_REVERB_L, text: "REVERB_L", audioUnit: audioUnit)
                SCButton(code: MCU_BUTTON_REVERB_R, text: "REVERB_R", audioUnit: audioUnit)
            }
            HStack {
                Button(action: {
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_PART_L), 1)
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_PART_R), 1)
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_PART_L), 0)
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_PART_R), 0)
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                    }
                }){ Text("DEMO") }
                
                Button(action: {
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_L), 1)
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 1)
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_L), 0)
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 0)
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                    }
                }){ Text("INIT ALL") }
                
                Button(action: {
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 1)
                    audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 1)
                    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_INST_R), 0)
                        audioUnit.lcd_SendButton(UInt8(MCU_BUTTON_POWER), 0)
                    }
                }){ Text("GS RESET") }
            }
            Button(action: {
                audioUnit.sc55_Reset()
            }) {
                Text("RESET")
            }
        }
    }
}

class Screen : ObservableObject {
    @Published
    var toggle: Bool = false
    public  var image: Image? {
        didSet {
            DispatchQueue.main.async { [weak self] in
                self?.toggle.toggle()
            }
        }
    }
    
    var renderer: Renderer
    private var displayLink: CVDisplayLink?
    
    let displayCallback: CVDisplayLinkOutputCallback = { displayLink, inNow, inOutputTime, flagsIn, flagsOut, displayLinkContext in
        let screen = unsafeBitCast(displayLinkContext, to: Screen.self)
        screen.renderer.draw()
        screen.image = Image(bitmap: screen.renderer.bitmap)
        return kCVReturnSuccess
    }
    
    init(width: Int, height: Int) {
        self.renderer = Renderer(width: width, height: height)

        let error = CVDisplayLinkCreateWithActiveCGDisplays(&self.displayLink)
        guard let link = self.displayLink, kCVReturnSuccess == error else {
            NSLog("Display Link created with error: %d", error)
            return
        }

        CVDisplayLinkSetOutputCallback(link, displayCallback, UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque()))
        CVDisplayLinkStart(link)
    }
    
    deinit {
        CVDisplayLinkStop(self.displayLink!)
    }
}
