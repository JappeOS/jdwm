import 'dart:typed_data';
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';

import 'package:jdwm/src/core/models/resize.dart';

import 'window_toolbar.dart';

class WindowEntry extends ChangeNotifier {
  final WindowEntryId id;
  final int? backendViewId;
  String? _monitorId;
  String _title;
  ImageProvider _icon;
  final Widget content;
  WindowChromeMode _chromeMode;
  final Size minSize;
  Rect _windowRect = Rect.zero;
  Widget? _toolbar;
  bool _maximized = false;
  bool _minimized = false;
  Rect? restoreRectAfterMaximize;
  String? restoreMonitorIdAfterMaximize;
  Rect? restoreRectAfterDock;
  String? restoreMonitorIdAfterDock;
  WindowDock _windowDock = WindowDock.normal;
  final bool allowResize;
  ResizeEdge? backendInteractiveResizeEdge;
  double backendContentInsetTop = 0;

  final GlobalKey repaintBoundaryKey = GlobalKey();

  String? get monitorId => _monitorId;
  String get title => _title;
  ImageProvider get icon => _icon;
  bool get usesToolbar => _chromeMode == WindowChromeMode.decorated;
  WindowChromeMode get chromeMode => _chromeMode;
  Rect get windowRect => _windowRect;
  Widget? get toolbar => _toolbar;
  bool get maximized => _maximized;
  bool get minimized => _minimized;
  WindowDock get windowDock => _windowDock;

  set monitorId(String? value) {
    if (_monitorId == value) return;
    _monitorId = value;
    notifyListeners();
  }

  set title(String value) {
    if (_title == value) return;
    _title = value;
    notifyListeners();
  }

  set icon(ImageProvider value) {
    if (_icon == value) return;
    _icon = value;
    notifyListeners();
  }

  set usesToolbar(bool value) {
    chromeMode =
        value ? WindowChromeMode.decorated : WindowChromeMode.borderless;
  }

  set chromeMode(WindowChromeMode value) {
    if (_chromeMode == value) return;
    _chromeMode = value;
    if (value == WindowChromeMode.decorated && _toolbar == null) {
      _toolbar = const DefaultWindowToolbar();
    }
    notifyListeners();
  }

  set windowRect(Rect value) {
    final aligned = Rect.fromLTRB(
      value.left.roundToDouble(),
      value.top.roundToDouble(),
      value.right.roundToDouble(),
      value.bottom.roundToDouble(),
    );
    if (_windowRect == aligned) return;
    _windowRect = aligned;
    notifyListeners();
  }

  set toolbar(Widget? value) {
    if (_toolbar == value) return;
    _toolbar = value;
    notifyListeners();
  }

  set maximized(bool value) {
    if (_maximized == value) return;
    _maximized = value;
    notifyListeners();
  }

  set minimized(bool value) {
    if (_minimized == value) return;
    _minimized = value;
    notifyListeners();
  }

  set windowDock(WindowDock value) {
    if (_windowDock == value) return;
    _windowDock = value;
    notifyListeners();
  }

  WindowEntry({
    String title = "",
    required ImageProvider icon,
    required this.content,
    this.backendViewId,
    bool usesToolbar = true,
    WindowChromeMode? chromeMode,
    Size initialSize = const Size(600, 480),
    this.minSize = const Size.square(100),
    this.allowResize = true,
  })  : id = WindowEntryId(),
        _title = title,
        _icon = icon,
        _chromeMode = chromeMode ??
            (usesToolbar
                ? WindowChromeMode.decorated
                : WindowChromeMode.borderless) {
    windowRect = Rect.fromLTWH(
      0,
      0,
      initialSize.width,
      initialSize.height,
    );
    if (_chromeMode == WindowChromeMode.decorated) {
      toolbar = const DefaultWindowToolbar();
    }
  }

  void toggleMaximize() {
    maximized = !maximized;
  }

  Future<Uint8List> getScreenshot() async {
    final box = repaintBoundaryKey.currentContext!.findRenderObject()
        as RenderRepaintBoundary;
    final image = await box.toImage();
    final byteData = await image.toByteData(
      format: ImageByteFormat.png,
    );

    return byteData!.buffer.asUint8List();
  }
}

/// Stable, unique identifier for a [WindowEntry].
///
/// Uses a monotonically incrementing integer so that [==] and [hashCode]
/// are consistent and unique for the lifetime of the process - safe to use
/// as a [Map] key.
class WindowEntryId {
  static int _nextId = 0;
  final int _id = _nextId++;

  @override
  bool operator ==(Object other) => other is WindowEntryId && _id == other._id;

  @override
  int get hashCode => _id;

  int compareTo(WindowEntryId other) => _id.compareTo(other._id);

  @override
  String toString() => _id.toString();
}

enum WindowDock {
  normal,
  topLeft,
  top,
  topRight,
  right,
  left,
  bottomLeft,
  bottom,
  bottomRight,
}

enum WindowChromeMode {
  decorated,
  borderless,
}
