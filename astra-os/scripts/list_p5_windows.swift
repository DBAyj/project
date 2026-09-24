import CoreGraphics
import Foundation

let expected = Set(["AstraOS Phone Display", "AstraOS Projection Display"])
let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] ?? []
let result: [[String: Any]] = windows.compactMap { window in
    guard let title = window[kCGWindowName as String] as? String,
          expected.contains(title),
          let number = window[kCGWindowNumber as String] as? Int,
          let bounds = window[kCGWindowBounds as String] as? [String: Any] else { return nil }
    return ["title": title, "window_id": number, "bounds": bounds]
}
let data = try JSONSerialization.data(withJSONObject: result, options: [.sortedKeys])
FileHandle.standardOutput.write(data)
FileHandle.standardOutput.write(Data("\n".utf8))
