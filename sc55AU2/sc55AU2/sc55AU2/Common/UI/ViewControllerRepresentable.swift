//
//  ViewControllerRepresentable.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import SwiftUI

#if os(iOS)
class WrapperVC: ViewController {
    override func loadView() {
        view = KitView()
    }
}

struct AUViewControllerUI: UIViewControllerRepresentable {
	var auViewController: UIViewController?

	init(viewController: UIViewController?) {
		self.auViewController = viewController
	}
	
	func makeUIViewController(context: Context) -> UIViewController {
		let viewController = WrapperVC()
		guard let auViewController = self.auViewController else {
			return viewController
		}
		
		viewController.addChild(auViewController)

		let frame: CGRect = viewController.view.bounds
		auViewController.view.frame = frame
		
		viewController.view.addSubview(auViewController.view)
		return viewController
	}
	
	func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
        // No opp
    }
}
#elseif os(macOS)
struct AUViewControllerUI: NSViewControllerRepresentable {
	
	var auViewController: NSViewController?

	init(viewController: NSViewController?) {
		self.auViewController = viewController
	}
	
	func makeNSViewController(context: Context) -> NSViewController {
		return self.auViewController!
	}
	
	func updateNSViewController(_ nsViewController: NSViewController, context: Context) {
        // No opp
	}
}
#endif
