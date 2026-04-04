import 'dart:async';

import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/core/models/resize.dart';
import 'package:jdwm/src/core/models/toplevel_decoration.dart';

import '../zenith_backend.dart';
import 'client_cursor.dart';
import 'monitor_region.dart';
import 'window_entry.dart';
import 'window_hierarchy.dart';

@immutable
class ManagedWindowInfo {
  const ManagedWindowInfo({
    required this.viewId,
    required this.protocol,
    required this.title,
    required this.appId,
    required this.visible,
    required this.maximized,
    required this.focused,
  });

  final int viewId;
  final String protocol;
  final String title;
  final String appId;
  final bool visible;
  final bool maximized;
  final bool focused;
}

class WindowManager extends StatefulWidget {
  final List<MonitorConfig> monitors;
  final List<MonitorConfig> Function(
    BuildContext context,
    List<MonitorConfig> backendMonitors,
  )? monitorLayoutBuilder;
  final Widget Function(BuildContext context, MonitorConfig monitor)?
      monitorBuilder;
  final Widget Function(BuildContext context, MonitorConfig monitor)?
      monitorOverlayBuilder;
  final bool enableZenithBackend;

  const WindowManager({
    super.key,
    this.monitors = const [],
    this.monitorLayoutBuilder,
    this.monitorBuilder,
    this.monitorOverlayBuilder,
    this.enableZenithBackend = true,
  });

  @override
  State<WindowManager> createState() => WindowManagerState();

  static WindowManagerState? of(BuildContext context) {
    return context.findAncestorStateOfType<WindowManagerState>();
  }
}

class WindowManagerState extends State<WindowManager> {
  static const _autoMonitorId = '__jdwm_auto_monitor__';

  final GlobalKey<WindowHierarchyState> _hierarchyKey = GlobalKey();
  final Map<String, GlobalKey> _regionKeys = {};
  final Map<int, WindowEntry> _backendWindows = {};
  final Map<int, List<ProviderSubscription>> _backendSubscriptions = {};
  final Map<int, VoidCallback> _backendFocusNodeDetachers = {};
  final Map<int, Rect> _lastRequestedBackendGeometries = {};
  final Set<int> _backendInitialPlacementDone = {};
  final Set<VoidCallback> _windowStateListeners = <VoidCallback>{};
  List<MonitorConfig> _effectiveMonitors = const [];

  Widget? _clientCursor;
  ProviderContainer? _backendContainer;
  ProviderSubscription? _windowMappedSubscription;
  ProviderSubscription? _windowUnmappedSubscription;
  ProviderSubscription? _monitorListSubscription;
  List<MonitorConfig> _backendMonitorConfigs = const [];
  bool? _cursorVisible;

  String _resolvedBackendWindowTitle({
    required int viewId,
    String fallback = '',
  }) {
    final state = _backendContainer!.read(xdgToplevelStatesProvider(viewId));
    final title = state.title.trim();
    if (title.isNotEmpty) {
      return state.title;
    }
    final appId = state.appId.trim();
    if (appId.isNotEmpty) {
      return state.appId;
    }
    return fallback;
  }

  double _backendClientInsetTopFor(WindowEntry entry) {
    if (entry.chromeMode != WindowChromeMode.decorated) {
      return 0;
    }
    return entry.backendContentInsetTop;
  }

  void addWindowStateListener(VoidCallback listener) {
    _windowStateListeners.add(listener);
  }

  void removeWindowStateListener(VoidCallback listener) {
    _windowStateListeners.remove(listener);
  }

  void _notifyWindowStateListeners() {
    for (final listener in _windowStateListeners.toList(growable: false)) {
      listener();
    }
  }

  @override
  void initState() {
    super.initState();
    _syncRegionKeys(widget.monitors);
    if (widget.enableZenithBackend) {
      _initZenithBackend();
    }
  }

  @override
  void didUpdateWidget(covariant WindowManager oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.monitors != widget.monitors) {
      _syncRegionKeys(widget.monitors);
    }
  }

  @override
  void dispose() {
    _windowMappedSubscription?.close();
    _windowUnmappedSubscription?.close();
    _monitorListSubscription?.close();
    for (final subs in _backendSubscriptions.values) {
      for (final sub in subs) {
        sub.close();
      }
    }
    for (final detach in _backendFocusNodeDetachers.values) {
      detach();
    }
    _backendFocusNodeDetachers.clear();
    _backendContainer?.dispose();
    super.dispose();
  }

  void _initZenithBackend() {
    _backendContainer = ProviderContainer();

    final platformApi = _backendContainer!.read(platformApiProvider.notifier);
    platformApi.init();
    unawaited(
      platformApi.requestMonitorsSnapshot().catchError((error) {
        if (error is PlatformException &&
            error.code == "method_does_not_exist") {
          return;
        }
        throw error;
      }),
    );
    platformApi.startWindowsMaximized(false);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      platformApi.startupComplete();
    });

    final initial = _backendContainer!.read(mappedWindowListProvider);
    for (final viewId in initial) {
      _addBackendWindow(viewId);
    }

    _windowMappedSubscription =
        _backendContainer!.listen(windowMappedStreamProvider, (_, next) {
      if (next case AsyncData<int>(:final value)) {
        _addBackendWindow(value);
      }
    });
    _windowUnmappedSubscription =
        _backendContainer!.listen(windowUnmappedStreamProvider, (_, next) {
      if (next case AsyncData<int>(:final value)) {
        _removeBackendWindow(value);
      }
    });

    final initialMonitors = _backendContainer!.read(backendMonitorListProvider);
    _setBackendMonitors(
      initialMonitors.map<MonitorConfig>((monitor) {
        return MonitorConfig(
          id: monitor.id,
          bounds: monitor.bounds,
          isPrimary: monitor.isPrimary,
        );
      }).toList(growable: false),
    );

    _monitorListSubscription =
        _backendContainer!.listen(backendMonitorListProvider, (_, next) {
      _setBackendMonitors(
        next.map<MonitorConfig>((monitor) {
          return MonitorConfig(
            id: monitor.id,
            bounds: monitor.bounds,
            isPrimary: monitor.isPrimary,
          );
        }).toList(growable: false),
      );
    });
  }

  void _setBackendMonitors(List<MonitorConfig> monitors) {
    if (_sameMonitorConfigs(_backendMonitorConfigs, monitors)) {
      return;
    }
    if (!mounted) {
      _backendMonitorConfigs = monitors;
      return;
    }
    setState(() {
      _backendMonitorConfigs = monitors;
    });
  }

  bool _sameMonitorConfigs(List<MonitorConfig> a, List<MonitorConfig> b) {
    if (identical(a, b)) {
      return true;
    }
    if (a.length != b.length) {
      return false;
    }
    for (var i = 0; i < a.length; i++) {
      if (a[i].id != b[i].id ||
          a[i].bounds != b[i].bounds ||
          a[i].margin != b[i].margin ||
          a[i].isPrimary != b[i].isPrimary) {
        return false;
      }
    }
    return true;
  }

  void _addBackendWindow(int viewId) {
    if (_backendWindows.containsKey(viewId)) {
      return;
    }

    final entry = WindowEntry(
      title: '',
      icon: const AssetImage('assets/cursor/grabbing.png'),
      backendViewId: viewId,
      content: XdgToplevelSurface(
        key: ValueKey('toplevel_$viewId'),
        viewId: viewId,
      ),
    );
    final initialDecoration =
        _backendContainer!.read(xdgToplevelStatesProvider(viewId)).decoration;
    entry.chromeMode = initialDecoration == ToplevelDecoration.serverSide
        ? WindowChromeMode.decorated
        : WindowChromeMode.borderless;
    entry.title = _resolvedBackendWindowTitle(
      viewId: viewId,
      fallback: entry.title,
    );
    _updateBackendWindowSizeFromState(viewId, entry);

    _backendWindows[viewId] = entry;
    pushWindow(entry);
    requestFocus(entry);

    _attachBackendListeners(viewId, entry);
    _notifyWindowStateListeners();
  }

  void _removeBackendWindow(int viewId) {
    final entry = _backendWindows.remove(viewId);
    if (entry != null) {
      popWindow(entry);
    }

    final subs = _backendSubscriptions.remove(viewId);
    if (subs != null) {
      for (final sub in subs) {
        sub.close();
      }
    }

    final detachFocus = _backendFocusNodeDetachers.remove(viewId);
    detachFocus?.call();
    _lastRequestedBackendGeometries.remove(viewId);
    _backendInitialPlacementDone.remove(viewId);
    _notifyWindowStateListeners();
  }

  void _attachBackendListeners(int viewId, WindowEntry entry) {
    final subs = <ProviderSubscription>[];
    final focusNode =
        _backendContainer!.read(xdgToplevelStatesProvider(viewId)).focusNode;
    void onFocusChange() {
      if (!mounted) {
        return;
      }
      if (focusNode.hasFocus) {
        requestFocus(entry);
      }
    }

    focusNode.addListener(onFocusChange);
    _backendFocusNodeDetachers[viewId] = () {
      focusNode.removeListener(onFocusChange);
    };

    subs.add(
      _backendContainer!.listen(
        xdgToplevelStatesProvider(viewId).select((v) => v.title),
        (_, next) {
          final resolved = _resolvedBackendWindowTitle(
            viewId: viewId,
            fallback: entry.title,
          );
          if (entry.title != resolved) {
            entry.title = resolved;
            _notifyWindowStateListeners();
          }
        },
        fireImmediately: true,
      ),
    );

    subs.add(
      _backendContainer!.listen(
        xdgToplevelStatesProvider(viewId).select((v) => v.appId),
        (_, __) {
          final resolved = _resolvedBackendWindowTitle(
            viewId: viewId,
            fallback: entry.title,
          );
          if (entry.title != resolved) {
            entry.title = resolved;
            _notifyWindowStateListeners();
          }
        },
        fireImmediately: true,
      ),
    );

    subs.add(
      _backendContainer!.listen(
        xdgToplevelStatesProvider(viewId).select((v) => v.visible),
        (_, next) {
          entry.minimized = !next;
          _notifyWindowStateListeners();
        },
        fireImmediately: true,
      ),
    );

    subs.add(
      _backendContainer!.listen(
        xdgToplevelStatesProvider(viewId).select((v) => v.maximized),
        (previous, next) {
          entry.maximized = next;
          final becameMaximized = previous != true && next;
          final becameRestored = previous == true && !next;
          if (becameMaximized) {
            entry.restoreRectAfterMaximize = entry.windowRect;
            entry.restoreMonitorIdAfterMaximize = entry.monitorId;
            _prepareBackendMaximizeForWindow(entry);
            _syncBackendManagedWindowSizes();
          } else if (becameRestored) {
            entry.windowDock = WindowDock.normal;
            final restoreRect = entry.restoreRectAfterMaximize;
            if (restoreRect != null) {
              entry.windowRect = restoreRect;
              syncBackendWindowGeometry(entry, force: true);
              entry.restoreRectAfterMaximize = null;
            }
            final restoreMonitorId = entry.restoreMonitorIdAfterMaximize;
            if (restoreMonitorId != null) {
              entry.monitorId = restoreMonitorId;
              entry.restoreMonitorIdAfterMaximize = null;
            }
          }
          _notifyWindowStateListeners();
        },
        fireImmediately: true,
      ),
    );

    subs.add(
      _backendContainer!.listen(
        xdgToplevelStatesProvider(viewId).select((v) => v.decoration),
        (_, next) {
          // Match legacy zenith_backend behavior:
          // - none/clientSide => app draws its own chrome (no JDWM decorations)
          // - serverSide => JDWM draws server-side decorations
          entry.chromeMode = next == ToplevelDecoration.serverSide
              ? WindowChromeMode.decorated
              : WindowChromeMode.borderless;
          if (entry.chromeMode != WindowChromeMode.decorated) {
            entry.backendContentInsetTop = 0;
          }
          _updateBackendWindowSizeFromState(viewId, entry);
          _notifyWindowStateListeners();
        },
        fireImmediately: true,
      ),
    );

    subs.add(
      _backendContainer!.listen(
        xdgSurfaceStatesProvider(viewId).select((v) => v.visibleBounds),
        (previous, next) {
          if (next.size.isEmpty) {
            return;
          }
          final current = entry.windowRect;
          var left = current.left;
          var top = current.top;

          final edge = entry.backendInteractiveResizeEdge;
          if (edge != null &&
              previous != null &&
              previous.size.isEmpty == false) {
            final offset = _computeWindowOffset(edge, previous.size, next.size);
            left += offset.dx;
            top += offset.dy;
          }

          final nextRect = Rect.fromLTWH(
            left,
            top,
            next.width,
            next.height,
          );
          if (!_backendInitialPlacementDone.contains(viewId)) {
            final placed =
                _applyInitialBackendWindowPlacement(entry, size: nextRect.size);
            if (placed) {
              _backendInitialPlacementDone.add(viewId);
              syncBackendWindowGeometry(entry, force: true);
              return;
            }
          }
          // Keep initial spawn placement constrained to monitor bounds, but
          // do not continuously clamp subsequent backend geometry updates.
          // Continuous clamping breaks cross-monitor interactive resize/move.
          entry.windowRect = nextRect;
          _notifyWindowStateListeners();
        },
        fireImmediately: true,
      ),
    );

    _backendSubscriptions[viewId] = subs;
  }

  void _updateBackendWindowSizeFromState(int viewId, WindowEntry entry) {
    final viewportRect = _backendViewportRectForEntry(viewId);
    if (viewportRect.size.isEmpty) {
      return;
    }

    final current = entry.windowRect;
    entry.windowRect = Rect.fromLTWH(
      current.left,
      current.top,
      viewportRect.width,
      viewportRect.height,
    );
  }

  Rect _backendViewportRectForEntry(int viewId) {
    return _backendContainer!
        .read(xdgSurfaceStatesProvider(viewId))
        .visibleBounds;
  }

  Offset _computeWindowOffset(ResizeEdge edge, Size oldSize, Size newSize) {
    final dx = newSize.width - oldSize.width;
    final dy = newSize.height - oldSize.height;
    switch (edge) {
      case ResizeEdge.topLeft:
        return Offset(-dx, -dy);
      case ResizeEdge.top:
      case ResizeEdge.topRight:
        return Offset(0, -dy);
      case ResizeEdge.left:
      case ResizeEdge.bottomLeft:
        return Offset(-dx, 0);
      case ResizeEdge.right:
      case ResizeEdge.bottomRight:
      case ResizeEdge.bottom:
        return Offset.zero;
    }
  }

  /// Returns the [MonitorConfig] whose bounds contain [position], or null.
  MonitorConfig? getMonitorAtPosition(Offset position) {
    for (final monitor in _effectiveMonitors) {
      if (monitor.bounds.contains(position)) {
        return monitor;
      }
    }
    return null;
  }

  /// Look up a [MonitorConfig] by id.
  MonitorConfig? getMonitorById(String id) {
    final monitors = _monitorsForPlacement();
    try {
      return monitors.firstWhere((m) => m.id == id);
    } on StateError {
      return null;
    }
  }

  /// Returns the [GlobalKey] for the [MonitorRegion] widget of the given
  /// monitor.
  GlobalKey? getRegionKey(String monitorId) => _regionKeys[monitorId];

  void setClientCursor(SystemMouseCursor cursor, Offset globalPosition) {
    setState(() {
      _clientCursor = Positioned(
        left: globalPosition.dx.round().toDouble(),
        top: globalPosition.dy.round().toDouble(),
        child: FractionalTranslation(
          translation: const Offset(-0.5, -0.5),
          child: ClientCursor.get(cursor),
        ),
      );
      setCursorVisible(false);
    });
  }

  void endClientCursor() {
    setState(() {
      _clientCursor = null;
      setCursorVisible(true);
    });
  }

  Future<void> setCursorVisible(bool visible) async {
    if (_backendContainer == null) {
      return;
    }
    if (_cursorVisible == visible) {
      return;
    }
    _cursorVisible = visible;
    await _backendContainer!
        .read(platformApiProvider.notifier)
        .setCursorVisible(visible);
  }

  Future<void> lockCursor(bool locked) async {
    if (_backendContainer == null) {
      return;
    }
    await _backendContainer!
        .read(platformApiProvider.notifier)
        .lockCursor(locked);
  }

  void pushWindow(WindowEntry entry, {String? monitorId}) {
    final monitors = _monitorsForPlacement();
    final targetMonitorId = monitorId ??
        (monitors.isNotEmpty ? _preferredPrimaryMonitor(monitors).id : null);
    if (targetMonitorId != null) {
      entry.monitorId = targetMonitorId;
    }
    _hierarchyKey.currentState?.pushWindowEntry(entry);
    _notifyWindowStateListeners();
  }

  void popWindow(WindowEntry entry) {
    final hierarchy = _hierarchyKey.currentState;
    if (hierarchy == null) {
      return;
    }
    final wasFocused = hierarchy.entriesByFocus.isNotEmpty &&
        hierarchy.entriesByFocus.last == entry;
    hierarchy.popWindowEntry(entry);

    if (wasFocused) {
      final remaining = hierarchy.entriesByFocus;
      if (remaining.isNotEmpty) {
        requestFocus(remaining.last);
      }
    }
    _notifyWindowStateListeners();
  }

  List<WindowEntry> getAllWindows() {
    return _hierarchyKey.currentState?.windows ?? [];
  }

  List<WindowEntry> getWindowsOnMonitor(String monitorId) {
    return _hierarchyKey.currentState?.windows
            .where((e) => e.monitorId == monitorId)
            .toList() ??
        [];
  }

  /// Unified focus path for both backend (CSD/SSD) and local windows.
  ///
  /// Ensures z-order focus is updated once and backend keyboard focus stays
  /// single-owner (no two active backend windows).
  void requestFocus(WindowEntry entry) {
    _hierarchyKey.currentState?.requestWindowFocus(entry);

    final container = _backendContainer;
    if (container == null) {
      return;
    }

    final targetViewId = entry.backendViewId;
    if (targetViewId != null) {
      container.read(platformApiProvider.notifier).activateWindow(
            targetViewId,
            true,
          );
    }
    for (final viewId in _backendWindows.keys) {
      final focusNode =
          container.read(xdgToplevelStatesProvider(viewId)).focusNode;
      if (targetViewId != null && viewId == targetViewId) {
        if (!focusNode.hasFocus) {
          focusNode.requestFocus();
        }
      } else {
        if (focusNode.hasFocus) {
          focusNode.unfocus();
        }
      }
    }

    _prepareBackendMaximizeForWindow(entry);
    _notifyWindowStateListeners();
  }

  List<ManagedWindowInfo> getManagedWindowsByFocus() {
    final container = _backendContainer;
    if (container == null) {
      return const <ManagedWindowInfo>[];
    }
    final focusedEntry = _hierarchyKey.currentState?.entriesByFocus.lastOrNull;
    final focusedViewId = focusedEntry?.backendViewId;
    final infos = <ManagedWindowInfo>[];
    for (final entry in _hierarchyKey.currentState?.entriesByFocus ??
        const <WindowEntry>[]) {
      final viewId = entry.backendViewId;
      if (viewId == null) {
        continue;
      }
      final state = container.read(xdgToplevelStatesProvider(viewId));
      final protocol =
          container.read(platformApiProvider.notifier).isXwaylandView(viewId)
              ? "xwayland"
              : "xdg";
      infos.add(
        ManagedWindowInfo(
          viewId: viewId,
          protocol: protocol,
          title: state.title,
          appId: state.appId,
          visible: state.visible,
          maximized: state.maximized,
          focused: focusedViewId == viewId,
        ),
      );
    }
    return infos;
  }

  void prepareBackendMaximizeForWindow(WindowEntry entry) {
    _prepareBackendMaximizeForWindow(entry);
  }

  MonitorConfig? _monitorForEntry(WindowEntry entry) {
    final monitors = _monitorsForPlacement();
    if (monitors.isEmpty) {
      return null;
    }
    final id = entry.monitorId;
    if (id != null) {
      for (final monitor in monitors) {
        if (monitor.id == id) {
          return monitor;
        }
      }
    }
    return _preferredPrimaryMonitor(monitors);
  }

  Rect targetRectForWindow(WindowEntry entry) {
    final monitor = _monitorForEntry(entry);
    if (monitor == null) {
      return entry.windowRect;
    }

    final usable = monitor.usableBounds;
    if (entry.maximized) {
      return usable;
    }

    switch (entry.windowDock) {
      case WindowDock.topLeft:
        return Rect.fromLTWH(
            usable.left, usable.top, usable.width / 2, usable.height / 2);
      case WindowDock.top:
        return Rect.fromLTWH(
            usable.left, usable.top, usable.width, usable.height / 2);
      case WindowDock.topRight:
        return Rect.fromLTWH(usable.left + usable.width / 2, usable.top,
            usable.width / 2, usable.height / 2);
      case WindowDock.left:
        return Rect.fromLTWH(
            usable.left, usable.top, usable.width / 2, usable.height);
      case WindowDock.right:
        return Rect.fromLTWH(usable.left + usable.width / 2, usable.top,
            usable.width / 2, usable.height);
      case WindowDock.bottomLeft:
        return Rect.fromLTWH(usable.left, usable.top + usable.height / 2,
            usable.width / 2, usable.height / 2);
      case WindowDock.bottom:
        return Rect.fromLTWH(usable.left, usable.top + usable.height / 2,
            usable.width, usable.height / 2);
      case WindowDock.bottomRight:
        return Rect.fromLTWH(
          usable.left + usable.width / 2,
          usable.top + usable.height / 2,
          usable.width / 2,
          usable.height / 2,
        );
      case WindowDock.normal:
        return Rect.fromLTWH(
          entry.windowRect.left,
          math.max(usable.top, entry.windowRect.top),
          math.max(entry.minSize.width, entry.windowRect.width),
          math.max(entry.minSize.height, entry.windowRect.height),
        );
    }
  }

  void _prepareBackendMaximizeForWindow(WindowEntry entry) {
    final backendViewId = entry.backendViewId;
    final container = _backendContainer;
    if (backendViewId == null || container == null) {
      return;
    }
    final monitor = _monitorForEntry(entry);
    if (monitor == null) {
      return;
    }
    final usable = monitor.usableBounds.size;
    final clientInsetTop = _backendClientInsetTopFor(entry);
    final clientHeight = math.max(
      entry.minSize.height,
      usable.height - clientInsetTop,
    );
    container.read(platformApiProvider.notifier).maximizedWindowSize(
          usable.width.round(),
          clientHeight.round(),
        );
  }

  void syncBackendWindowGeometry(WindowEntry entry, {bool force = false}) {
    final backendViewId = entry.backendViewId;
    final container = _backendContainer;
    if (backendViewId == null || container == null) {
      return;
    }
    final target = targetRectForWindow(entry);
    final clientInsetTop = _backendClientInsetTopFor(entry);
    var clientHeight = target.height;
    // In maximized/docked states, target rect represents outer frame bounds.
    // Convert to backend client content geometry by subtracting SSD headerbar.
    if (clientInsetTop > 0 &&
        (entry.maximized || entry.windowDock != WindowDock.normal)) {
      clientHeight = math.max(
        entry.minSize.height,
        target.height - clientInsetTop,
      );
    }
    final desiredGeometry = Rect.fromLTWH(
      target.left.roundToDouble(),
      (target.top + clientInsetTop).roundToDouble(),
      target.width.roundToDouble(),
      clientHeight.roundToDouble(),
    );
    final lastSent = _lastRequestedBackendGeometries[backendViewId];
    if (!force && lastSent != null && lastSent == desiredGeometry) {
      return;
    }
    container.read(xdgToplevelStatesProvider(backendViewId).notifier).resize(
          desiredGeometry.width.round(),
          desiredGeometry.height.round(),
          x: desiredGeometry.left,
          y: desiredGeometry.top,
        );
    _lastRequestedBackendGeometries[backendViewId] = desiredGeometry;
  }

  void _syncBackendManagedWindowSizes() {
    if (_backendContainer == null || _effectiveMonitors.isEmpty) {
      return;
    }
    for (final entry in _backendWindows.values) {
      if (!entry.maximized && entry.windowDock == WindowDock.normal) {
        continue;
      }
      syncBackendWindowGeometry(entry);
    }
  }

  List<MonitorConfig> _monitorsForPlacement() {
    if (_effectiveMonitors.isNotEmpty) {
      return _effectiveMonitors;
    }
    if (_backendMonitorConfigs.isNotEmpty) {
      return _backendMonitorConfigs;
    }
    if (widget.monitors.isNotEmpty) {
      return widget.monitors;
    }
    return const [];
  }

  MonitorConfig _preferredPrimaryMonitor(List<MonitorConfig> monitors) {
    for (final monitor in monitors) {
      if (monitor.isPrimary) {
        return monitor;
      }
    }
    return monitors.first;
  }

  MonitorConfig? _focusedMonitor(List<MonitorConfig> monitors) {
    final focused = _hierarchyKey.currentState?.entriesByFocus.lastOrNull;
    if (focused == null) {
      return null;
    }
    final focusedMonitorId = focused.monitorId;
    if (focusedMonitorId == null) {
      return null;
    }
    for (final monitor in monitors) {
      if (monitor.id == focusedMonitorId) {
        return monitor;
      }
    }
    return null;
  }

  Rect _clampRectToMonitorUsableBounds(WindowEntry entry, Rect rect) {
    final monitor = _monitorForEntry(entry);
    if (monitor == null) {
      return rect;
    }
    final usable = monitor.usableBounds;
    if (usable.width <= 0 || usable.height <= 0) {
      return rect;
    }

    final maxLeft = math.max(usable.left, usable.right - rect.width);
    final maxTop = math.max(usable.top, usable.bottom - rect.height);
    final clampedLeft = rect.left.clamp(usable.left, maxLeft).toDouble();
    final clampedTop = rect.top.clamp(usable.top, maxTop).toDouble();
    return Rect.fromLTWH(
      clampedLeft,
      clampedTop,
      rect.width,
      rect.height,
    );
  }

  bool _applyInitialBackendWindowPlacement(WindowEntry entry, {Size? size}) {
    final monitors = _monitorsForPlacement();
    if (monitors.isEmpty) {
      return false;
    }

    var selectedMonitor =
        entry.monitorId != null ? getMonitorById(entry.monitorId!) : null;
    selectedMonitor ??= _focusedMonitor(monitors);
    selectedMonitor ??= _preferredPrimaryMonitor(monitors);
    entry.monitorId = selectedMonitor.id;

    final windowSize = size ?? entry.windowRect.size;
    final usable = selectedMonitor.usableBounds;
    final windowsOnMonitor = getWindowsOnMonitor(selectedMonitor.id)
        .where((window) => window.id != entry.id)
        .length;

    Rect preferredRect;
    if (windowsOnMonitor == 0) {
      preferredRect = Rect.fromLTWH(
        usable.left + ((usable.width - windowSize.width) / 2),
        usable.top + ((usable.height - windowSize.height) / 2),
        windowSize.width,
        windowSize.height,
      );
    } else {
      const cascadeX = 28.0;
      const cascadeY = 24.0;
      final cascadeStep = (windowsOnMonitor - 1) % 10;
      preferredRect = Rect.fromLTWH(
        usable.left + cascadeX * (cascadeStep + 1),
        usable.top + cascadeY * (cascadeStep + 1),
        windowSize.width,
        windowSize.height,
      );
    }

    entry.windowRect = _clampRectToMonitorUsableBounds(entry, preferredRect);
    return true;
  }

  void _applyPendingInitialBackendWindowPlacements() {
    if (_backendContainer == null) {
      return;
    }
    for (final backendEntry in _backendWindows.entries) {
      final viewId = backendEntry.key;
      if (_backendInitialPlacementDone.contains(viewId)) {
        continue;
      }
      final viewportRect = _backendViewportRectForEntry(viewId);
      final placed = _applyInitialBackendWindowPlacement(
        backendEntry.value,
        size: viewportRect.size.isEmpty ? null : viewportRect.size,
      );
      if (placed) {
        _backendInitialPlacementDone.add(viewId);
        syncBackendWindowGeometry(backendEntry.value, force: true);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (BuildContext context, BoxConstraints constraints) {
        final monitors = _resolveMonitors(constraints);
        _effectiveMonitors = monitors;
        _syncRegionKeys(monitors);
        _reconcileWindowMonitorAssignments(monitors);
        _applyPendingInitialBackendWindowPlacements();
        _syncBackendManagedWindowSizes();

        final stack = Stack(
          children: [
            // Layer 1: per-monitor background (rootWindow)
            if (widget.monitorBuilder != null) ...[
              ...monitors.map((monitor) {
                return Positioned(
                  left: monitor.bounds.left,
                  top: monitor.bounds.top,
                  width: monitor.bounds.width,
                  height: monitor.bounds.height,
                  child: widget.monitorBuilder!(context, monitor),
                );
              }),
            ],

            // Layer 2: per-monitor regions (spatial anchors for coordinate
            // conversion; no window state lives here)
            for (final monitor in monitors)
              Positioned(
                left: monitor.bounds.left,
                top: monitor.bounds.top,
                child: MonitorRegion(
                  config: monitor,
                  regionKey: _regionKeys[monitor.id]!,
                  child: const SizedBox.shrink(),
                ),
              ),

            // Layer 3: the single window stack, sized to the full global area
            Positioned.fill(
              child: WindowHierarchy(
                key: _hierarchyKey,
              ),
            ),

            // Layer 4: popups from the backend compositor
            if (widget.enableZenithBackend) const PopupStack(),

            // Layer 5: per-monitor overlays (e.g. monitor-specific toolbars)
            if (widget.monitorOverlayBuilder != null) ...[
              ...monitors.map((monitor) {
                return Positioned(
                  left: monitor.bounds.left,
                  top: monitor.bounds.top,
                  width: monitor.bounds.width,
                  height: monitor.bounds.height,
                  child: widget.monitorOverlayBuilder!(context, monitor),
                );
              }),
            ],

            // Layer 6: client cursor overlay
            if (_clientCursor != null) ...[
              _clientCursor!,
            ]
          ],
        );

        if (!widget.enableZenithBackend) {
          return stack;
        }

        if (_backendContainer == null) {
          return stack;
        }

        return UncontrolledProviderScope(
          container: _backendContainer!,
          child: stack,
        );
      },
    );
  }

  List<MonitorConfig> _resolveMonitors(BoxConstraints constraints) {
    if (widget.monitors.isNotEmpty) {
      return widget.monitors;
    }

    final backendOrDynamic = _backendMonitorConfigs.isNotEmpty
        ? _backendMonitorConfigs
        : [
            MonitorConfig(
              id: _autoMonitorId,
              bounds: Rect.fromLTWH(
                0,
                0,
                constraints.maxWidth,
                constraints.maxHeight,
              ),
              isPrimary: true,
            ),
          ];

    final layoutBuilder = widget.monitorLayoutBuilder;
    if (layoutBuilder != null) {
      return layoutBuilder(context, backendOrDynamic);
    }
    return backendOrDynamic;
  }

  void _syncRegionKeys(List<MonitorConfig> monitors) {
    final monitorIds = monitors.map((m) => m.id).toSet();
    _regionKeys.removeWhere((id, _) => !monitorIds.contains(id));
    for (final monitor in monitors) {
      _regionKeys.putIfAbsent(monitor.id, GlobalKey.new);
    }
  }

  void _reconcileWindowMonitorAssignments(List<MonitorConfig> monitors) {
    if (monitors.isEmpty) {
      return;
    }
    final defaultMonitorId = monitors.first.id;
    final monitorIds = monitors.map((m) => m.id).toSet();
    for (final window
        in _hierarchyKey.currentState?.windows ?? const <WindowEntry>[]) {
      final id = window.monitorId;
      if (id == null || !monitorIds.contains(id)) {
        window.monitorId = defaultMonitorId;
      }
    }
  }
}

/// Immutable description of one physical monitor.
class MonitorConfig {
  final String id;
  final Rect bounds; // Position and size in global coordinates
  final EdgeInsets? margin;
  final bool isPrimary;

  const MonitorConfig({
    required this.id,
    required this.bounds,
    this.margin,
    this.isPrimary = false,
  });

  MonitorConfig copyWith({
    String? id,
    Rect? bounds,
    EdgeInsets? margin,
    bool? isPrimary,
  }) {
    return MonitorConfig(
      id: id ?? this.id,
      bounds: bounds ?? this.bounds,
      margin: margin ?? this.margin,
      isPrimary: isPrimary ?? this.isPrimary,
    );
  }

  /// The usable area inside [bounds] after applying [margin].
  Rect get usableBounds {
    final m = margin ?? EdgeInsets.zero;
    return Rect.fromLTRB(
      bounds.left + m.left,
      bounds.top + m.top,
      bounds.right - m.right,
      bounds.bottom - m.bottom,
    );
  }

  /// Usable size (convenience).
  Size get usableSize => usableBounds.size;
}
