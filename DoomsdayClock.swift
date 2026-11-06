import AppKit
import CoreGraphics

// Local fan wallpaper. The countdown uses a fixed WITA calendar, including
// calendar months, so it agrees with the five fields used by Marvel's clock.
enum Countdown {
    static var calendar: Calendar {
        var c = Calendar(identifier: .gregorian)
        c.timeZone = TimeZone(identifier: "Asia/Makassar")!
        return c
    }
    static let defaultTarget = calendar.date(from: DateComponents(year: 2026, month: 12, day: 16))!
    static func parts(now: Date, target: Date) -> [Int] {
        guard target > now else { return [0, 0, 0, 0, 0] }
        let wholeSecond = Date(timeIntervalSince1970: floor(now.timeIntervalSince1970))
        let d = calendar.dateComponents([.month, .day, .hour, .minute, .second], from: wholeSecond, to: target)
        return [d.month ?? 0, d.day ?? 0, d.hour ?? 0, d.minute ?? 0, d.second ?? 0]
    }
    static func dateLabel(_ date: Date) -> String {
        let f = DateFormatter()
        f.locale = Locale(identifier: "id_ID")
        f.calendar = calendar
        f.timeZone = calendar.timeZone
        f.dateFormat = "d MMMM yyyy · HH.mm 'WITA'"
        return f.string(from: date)
    }
}

final class ClockView: NSView {
    override var isFlipped: Bool { true }
    var target = Countdown.defaultTarget
    var transparentBackground = true
    var bottomClearance: CGFloat = 150
    var renderDate: Date?
    override init(frame: NSRect) {
        super.init(frame: frame)
        setAccessibilityElement(true)
        setAccessibilityRole(.image)
        setAccessibilityLabel("Avengers Doomsday countdown")
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    private func centeredText(_ text: String, y: CGFloat, size: CGFloat, color: NSColor, tracking: CGFloat = 0, weight: NSFont.Weight = .medium, centerX: CGFloat = 960) {
        let attrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.systemFont(ofSize: size, weight: weight),
            .foregroundColor: color,
            .kern: tracking
        ]
        let s = NSAttributedString(string: text, attributes: attrs)
        let width = s.size().width
        s.draw(at: NSPoint(x: centerX - width / 2, y: y))
    }
    override func draw(_ dirtyRect: NSRect) {
        // Clear old glyphs every frame so transparency never leaves trails.
        NSColor.clear.setFill()
        bounds.fill(using: .copy)
        if !transparentBackground {
            NSColor(calibratedWhite: 0.035, alpha: 1).setFill()
            bounds.fill()
        }
        let ctx = NSGraphicsContext.current!.cgContext
        ctx.saveGState()
        // Preserve the existing text size on this Mac, while keeping the
        // whole block above the Dock and within narrow displays.
        let scale = min(max(bounds.width / 1920, bounds.height / 1080), bounds.width / 1380)
        ctx.translateBy(x: (bounds.width - 1920 * scale) / 2,
                        y: bounds.height - bottomClearance - 1024 * scale)
        ctx.scaleBy(x: scale, y: scale)
        ctx.setShadow(offset: CGSize(width: 0, height: 2), blur: 8,
                      color: NSColor.black.withAlphaComponent(0.75).cgColor)
        let now = renderDate ?? Date()
        let values = Countdown.parts(now: now, target: target)
        let white = NSColor.white
        let labels = ["MONTHS", "DAYS", "HOURS", "MINUTES", "SECONDS"]
        let startX: CGFloat = 498
        let groupWidth: CGFloat = 164
        let step: CGFloat = 190
        // Keep the numbers visibly subordinate to the title, using the same
        // font face and a cap height of 70% of the title's cap height.
        let baseFont = NSFont(name: "AvenirNext-Bold", size: 100) ?? NSFont.boldSystemFont(ofSize: 100)
        let titleHeight: CGFloat = 74
        let titleFont = NSFont(name: baseFont.fontName, size: 100 * titleHeight / baseFont.capHeight) ?? baseFont
        let title = NSAttributedString(string: "DOOMSDAY IS COMING", attributes: [.font: titleFont, .foregroundColor: white])
        title.draw(at: NSPoint(x: 960 - title.size().width / 2,
                               y: 748 - (titleFont.ascender - titleFont.capHeight)))
        let numeralHeight: CGFloat = titleHeight * 0.70
        let pointSize = 100 * numeralHeight / baseFont.capHeight
        let descriptor = baseFont.fontDescriptor.addingAttributes([
            .featureSettings: [[NSFontDescriptor.FeatureKey.typeIdentifier: 6,
                                NSFontDescriptor.FeatureKey.selectorIdentifier: 0]]
        ])
        let numeralFont = NSFont(descriptor: descriptor, size: pointSize) ?? baseFont
        let numeralY: CGFloat = 872 - (numeralFont.ascender - numeralFont.capHeight)
        for i in 0..<5 {
            let x = startX + CGFloat(i) * step
            let color = white
            let number = NSAttributedString(string: String(format: "%02d", values[i]), attributes: [.font: numeralFont, .foregroundColor: color])
            number.draw(at: NSPoint(x: x + groupWidth / 2 - number.size().width / 2, y: numeralY))
            centeredText(labels[i], y: 952, size: 19, color: white, tracking: 3.2, centerX: x + groupWidth / 2)
            if i < 4 {
                let colon = NSAttributedString(string: ":", attributes: [.font: numeralFont, .foregroundColor: color])
                colon.draw(at: NSPoint(x: x + groupWidth / 2 + step / 2 - colon.size().width / 2, y: numeralY))
            }
        }
        if target <= now {
            centeredText("DOOMSDAY IS HERE", y: 1043, size: 18, color: white, tracking: 5)
        }
        ctx.restoreGState()
    }
}

final class DesktopWindow: NSWindow {
    override var canBecomeKey: Bool { false }
    override var canBecomeMain: Bool { false }
}

final class AppDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate, NSWindowDelegate {
    var status: NSStatusItem!
    var desktop: NSWindow?
    var widget: NSWindow?
    var preview: NSWindow?
    var timer: Timer?
    let settings = UserDefaults.standard
    var target: Date {
        let time = settings.double(forKey: "target")
        return time > 0 ? Date(timeIntervalSince1970: time) : Countdown.defaultTarget
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        // Reopening the bundle must not leave multiple wallpaper processes.
        if let id = Bundle.main.bundleIdentifier,
           NSRunningApplication.runningApplications(withBundleIdentifier: id).contains(where: { $0.processIdentifier != ProcessInfo.processInfo.processIdentifier }) {
            NSApp.terminate(nil)
            return
        }
        NSApp.setActivationPolicy(.accessory)
        status = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        status.button?.title = " DOOM"
        status.button?.image = NSImage(systemSymbolName: "clock", accessibilityDescription: "Doomsday Clock")
        status.button?.imagePosition = .imageLeading
        status.button?.toolTip = "Doomsday · " + Countdown.dateLabel(target)
        let menu = NSMenu()
        menu.delegate = self
        status.menu = menu
        rebuildMenu()
        showDesktop()
        if settings.bool(forKey: "widget") { showWidget() }
        timer = Timer(timeInterval: 1, target: self, selector: #selector(tick), userInfo: nil, repeats: true)
        timer?.tolerance = 0.08
        RunLoop.main.add(timer!, forMode: .common)
        NotificationCenter.default.addObserver(self, selector: #selector(screenChanged), name: NSApplication.didChangeScreenParametersNotification, object: nil)
        NSWorkspace.shared.notificationCenter.addObserver(self, selector: #selector(tick), name: NSWorkspace.didWakeNotification, object: nil)
    }
    func menuWillOpen(_ menu: NSMenu) { rebuildMenu() }
    func item(_ title: String, _ action: Selector? = nil, checked: Bool = false) -> NSMenuItem {
        let i = NSMenuItem(title: title, action: action, keyEquivalent: "")
        i.target = self
        if checked { i.state = .on }
        return i
    }
    func rebuildMenu() {
        guard let menu = status.menu else { return }
        menu.removeAllItems()
        let title = item("AVENGERS: DOOMSDAY")
        title.isEnabled = false
        menu.addItem(title)
        let date = item(Countdown.dateLabel(target))
        date.isEnabled = false
        menu.addItem(date)
        menu.addItem(.separator())
        menu.addItem(item("Wallpaper desktop", #selector(toggleDesktop), checked: desktop?.isVisible == true))
        menu.addItem(item("Widget mengambang", #selector(toggleWidget), checked: widget?.isVisible == true))
        menu.addItem(item("Pratinjau besar…", #selector(showPreview)))
        menu.addItem(.separator())
        menu.addItem(item("Ubah tanggal countdown…", #selector(changeDate)))
        menu.addItem(item("Reset ke 16 Desember 2026", #selector(resetDate)))
        menu.addItem(.separator())
        menu.addItem(item("Livestream resmi Marvel ↗", #selector(openReference)))
        menu.addItem(item("Petunjuk penggunaan…", #selector(showHelp)))
        let quit = item("Keluar dari Doomsday Clock", #selector(quit))
        quit.keyEquivalent = "q"
        menu.addItem(quit)
    }
    func newView(_ frame: NSRect) -> ClockView {
        let v = ClockView(frame: frame)
        v.target = target
        v.transparentBackground = true
        v.autoresizingMask = [.width, .height]
        return v
    }
    func showDesktop() {
        guard let screen = NSScreen.screens.first else { return }
        let w = DesktopWindow(contentRect: screen.frame, styleMask: .borderless, backing: .buffered, defer: false)
        w.title = "Doomsday Wallpaper"
        w.level = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.desktopWindow)) + 1)
        w.collectionBehavior = [.canJoinAllSpaces, .stationary, .ignoresCycle]
        w.isOpaque = false
        w.backgroundColor = .clear
        w.hasShadow = false
        w.ignoresMouseEvents = true
        w.isReleasedWhenClosed = false
        let view = newView(NSRect(origin: .zero, size: screen.frame.size))
        view.bottomClearance = max(150, screen.visibleFrame.minY - screen.frame.minY + 48)
        w.contentView = view
        desktop = w
        w.orderFrontRegardless()
    }
    @objc func screenChanged() {
        let visible = desktop?.isVisible == true
        desktop?.close()
        desktop = nil
        if visible { showDesktop() }
    }
    @objc func toggleDesktop() {
        if let desktop, desktop.isVisible { desktop.orderOut(nil) }
        else if let desktop { desktop.orderFrontRegardless() }
        else { showDesktop() }
    }
    func showWidget() {
        if let widget { widget.orderFrontRegardless(); return }
        let frame = NSRect(x: 80, y: 100, width: 560, height: 315)
        let w = NSWindow(contentRect: frame, styleMask: .borderless, backing: .buffered, defer: false)
        w.title = "Doomsday Widget"
        w.level = .floating
        w.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
        w.isMovableByWindowBackground = true
        w.isOpaque = false
        w.backgroundColor = .clear
        w.isReleasedWhenClosed = false
        let view = newView(NSRect(origin: .zero, size: frame.size))
        view.bottomClearance = 32
        w.contentView = view
        w.setFrameAutosaveName("DoomsdayWidgetPosition")
        widget = w
        w.orderFrontRegardless()
    }
    @objc func toggleWidget() {
        if let widget, widget.isVisible {
            widget.orderOut(nil)
            settings.set(false, forKey: "widget")
        } else {
            showWidget()
            settings.set(true, forKey: "widget")
        }
    }
    @objc func showPreview() {
        if let preview { preview.makeKeyAndOrderFront(nil); NSApp.activate(); return }
        let frame = NSRect(x: 0, y: 0, width: 1120, height: 630)
        let w = NSWindow(contentRect: frame, styleMask: [.titled, .closable, .miniaturizable, .resizable], backing: .buffered, defer: false)
        w.title = "Avengers: Doomsday · " + Countdown.dateLabel(target)
        w.minSize = NSSize(width: 640, height: 380)
        w.contentAspectRatio = NSSize(width: 16, height: 9)
        w.isReleasedWhenClosed = false
        let view = newView(NSRect(origin: .zero, size: frame.size))
        view.bottomClearance = 32
        w.contentView = view
        view.transparentBackground = false
        w.center()
        preview = w
        w.makeKeyAndOrderFront(nil)
        NSApp.activate()
    }
    @objc func tick() {
        for w in [desktop, widget, preview].compactMap({ $0 }) where w.isVisible {
            (w.contentView as? ClockView)?.needsDisplay = true
        }
    }
    func updateViews() {
        for w in [desktop, widget, preview].compactMap({ $0 }) {
            if let v = w.contentView as? ClockView {
                v.target = target
                v.needsDisplay = true
            }
        }
        status.button?.toolTip = "Doomsday · " + Countdown.dateLabel(target)
        preview?.title = "Avengers: Doomsday · " + Countdown.dateLabel(target)
    }
    @objc func changeDate() {
        let alert = NSAlert()
        alert.messageText = "Tanggal countdown"
        alert.informativeText = "Pilih tanggal dan jam dalam WITA (Asia/Makassar)."
        alert.addButton(withTitle: "Simpan")
        alert.addButton(withTitle: "Batal")
        let picker = NSDatePicker(frame: NSRect(x: 0, y: 0, width: 310, height: 32))
        picker.datePickerStyle = .textFieldAndStepper
        picker.datePickerElements = [.yearMonthDay, .hourMinute]
        picker.timeZone = Countdown.calendar.timeZone
        picker.calendar = Countdown.calendar
        picker.dateValue = target
        alert.accessoryView = picker
        NSApp.activate()
        if alert.runModal() == .alertFirstButtonReturn {
            settings.set(picker.dateValue.timeIntervalSince1970, forKey: "target")
            updateViews()
        }
    }
    @objc func resetDate() {
        settings.removeObject(forKey: "target")
        updateViews()
    }
    @objc func openReference() {
        NSWorkspace.shared.open(URL(string: "https://www.youtube.com/watch?v=f17J3AXVK5w")!)
    }
    @objc func showHelp() {
        let a = NSAlert()
        a.messageText = "Doomsday Clock"
        a.informativeText = "Countdown lokal menuju 16 Desember 2026, 00.00 WITA.\n\nCountdown transparan tampil di atas wallpaper Mac milikmu, di belakang ikon desktop. Buka desktop untuk melihatnya. Gunakan menu DOOM untuk mematikan wallpaper atau menyalakan widget yang bisa diseret.\n\nPratinjau besar dapat dijadikan layar penuh dengan tombol hijau macOS. Keluar akan langsung mengembalikan tampilan wallpaper sebelumnya.\n\nDoomsday Clock adalah proyek penggemar Avengers: Doomsday."
        a.addButton(withTitle: "Mengerti")
        NSApp.activate()
        a.runModal()
    }
    @objc func quit() { NSApp.terminate(nil) }
}

let args = CommandLine.arguments
if args.contains("--self-test") {
    let c = Countdown.calendar
    func date(_ y: Int, _ m: Int, _ d: Int, _ h: Int = 0, _ min: Int = 0, _ sec: Int = 0) -> Date {
        c.date(from: DateComponents(year: y, month: m, day: d, hour: h, minute: min, second: sec))!
    }
    let t = Countdown.defaultTarget
    assert(ISO8601DateFormatter().string(from: t) == "2026-12-15T16:00:00Z")
    assert(Countdown.parts(now: date(2026,11,5), target: t) == [1,11,0,0,0])
    assert(Countdown.parts(now: date(2026,12,15,23,59,59), target: t) == [0,0,0,0,1])
    assert(Countdown.parts(now: date(2026,12,16), target: t) == [0,0,0,0,0])
    assert(Countdown.parts(now: date(2027,1,1), target: t) == [0,0,0,0,0])
    assert(Countdown.parts(now: date(2026,11,16), target: t) == [1,0,0,0,0])
    print("PASS: WITA target, calendar month, second rollover, zero, expiry")
} else if let idx = args.firstIndex(of: "--render"), args.count > idx + 1 {
    let width = args.contains("--mac-screen") ? 1470 : 1920
    let height = args.contains("--mac-screen") ? 956 : 1080
    let v = ClockView(frame: NSRect(x: 0, y: 0, width: width, height: height))
    v.transparentBackground = !args.contains("--opaque-preview")
    let rep = v.bitmapImageRepForCachingDisplay(in: v.bounds)!
    v.cacheDisplay(in: v.bounds, to: rep)
    try rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: args[idx + 1]))
    print("Rendered \(width)×\(height) wallpaper preview")
} else {
    let app = NSApplication.shared
    let delegate = AppDelegate()
    app.delegate = delegate
    app.run()
}
