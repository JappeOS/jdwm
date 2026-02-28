import 'dart:async';
import 'dart:math';

import 'package:flutter/gestures.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart' as provider_pkg hide Consumer;
import 'package:shadcn_flutter/shadcn_flutter.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/core/models/resize.dart';
import 'package:jdwm/util/rect_overflow_box.dart';

import 'window_manager.dart';
import 'window_entry.dart';
import 'window_hierarchy.dart';
import 'window_resize_gesture_detector.dart';

class TitlebarDragCallbacks extends InheritedWidget {
  final void Function(DragDownDetails) onDragStart;
  final void Function(DragUpdateDetails) onDrag;
  final void Function(DragEndDetails) onDragEnd;

  const TitlebarDragCallbacks({
    super.key,
    required this.onDragStart,
    required this.onDrag,
    required this.onDragEnd,
    required super.child,
  });

  static TitlebarDragCallbacks? of(BuildContext context) {
    return context.dependOnInheritedWidgetOfExactType<TitlebarDragCallbacks>();
  }

  @override
  bool updateShouldNotify(TitlebarDragCallbacks oldWidget) => true;
}

class Window extends StatefulWidget {
  final WindowEntry entry;

  const Window({
    required super.key,
    required this.entry,
  });

  @override
  _WindowState createState() => _WindowState();

  @override
  String toString({DiagnosticLevel minLevel = DiagnosticLevel.info}) =>
      entry.title;
}

class _WindowState extends State<Window> {
  static const double _resizingSpacing = 8;
  static const double _dockThickness = 10;

  final GlobalKey _mainContainerKey = GlobalKey();
  final GlobalKey _toolbarMeasureKey = GlobalKey();
  Offset? _lastDragGlobalPosition;
  double? _dragRawLeft, _dragRawTop;
  double? _rawLeft, _rawTop, _rawRight, _rawBottom;
  Size? _backendResizeStartSize;
  Offset _backendResizeAccumDelta = Offset.zero;
  ResizeEdge? _interactiveResizeRequestedForEdge;
  ProviderSubscription? _backendInteractiveMoveSubscription;
  ProviderSubscription? _backendInteractiveResizeSubscription;
  bool _backendInteractiveMoveActive = false;
  ResizeEdge? _backendInteractiveResizeActiveEdge;
  bool _backendInteractionListenersAttached = false;
  double _measuredToolbarHeight = 0;
  bool _backendResizeRequestInFlight = false;
  _BackendResizeRequest? _pendingBackendResizeRequest;
  Timer? _backendResizeDispatchTimer;
  int _lastBackendResizeDispatchMicros = 0;

  void _cancelBackendResizeDispatchTimer() {
    _backendResizeDispatchTimer?.cancel();
    _backendResizeDispatchTimer = null;
  }

  double _backendClientInsetTopFor(WindowEntry entry) {
    if (entry.backendViewId == null ||
        entry.chromeMode != WindowChromeMode.decorated) {
      return 0;
    }
    return entry.backendContentInsetTop;
  }

  @override
  void initState() {
    super.initState();
    GestureBinding.instance.pointerRouter.addGlobalRoute(_onGlobalPointerEvent);
  }

  int? get _backendViewId => widget.entry.backendViewId;

  ProviderContainer? _backendContainer(BuildContext context) {
    if (_backendViewId == null) {
      return null;
    }
    try {
      return ProviderScope.containerOf(context, listen: false);
    } catch (_) {
      return null;
    }
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _attachBackendInteractionListenersIfNeeded();
  }

  @override
  void dispose() {
    GestureBinding.instance.pointerRouter
        .removeGlobalRoute(_onGlobalPointerEvent);
    _cancelBackendResizeDispatchTimer();
    _pendingBackendResizeRequest = null;
    _backendInteractiveMoveSubscription?.close();
    _backendInteractiveResizeSubscription?.close();
    super.dispose();
  }

  void _scheduleToolbarMeasure() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) {
        return;
      }
      final box =
          _toolbarMeasureKey.currentContext?.findRenderObject() as RenderBox?;
      if (box == null || !box.hasSize) {
        return;
      }
      final height = box.size.height;
      if (!height.isFinite) {
        return;
      }
      if ((height - _measuredToolbarHeight).abs() < 0.5) {
        return;
      }
      final entry = widget.entry;
      if (entry.backendViewId != null &&
          entry.chromeMode == WindowChromeMode.decorated) {
        entry.backendContentInsetTop = height;
      }
      setState(() {
        _measuredToolbarHeight = height;
      });
      final manager = WindowManager.of(context);
      if (manager != null && entry.backendViewId != null) {
        manager.syncBackendWindowGeometry(entry, force: true);
      }
    });
  }

  void _attachBackendInteractionListenersIfNeeded() {
    if (_backendInteractionListenersAttached) {
      return;
    }
    final viewId = _backendViewId;
    if (viewId == null) {
      return;
    }
    final container = _backendContainer(context);
    if (container == null) {
      return;
    }
    _backendInteractiveMoveSubscription = container.listen(
      xdgToplevelStatesProvider(viewId)
          .select((v) => v.interactiveMoveRequested),
      (_, __) {
        _backendInteractiveMoveActive = true;
        _backendInteractiveResizeActiveEdge = null;
        widget.entry.backendInteractiveResizeEdge = null;
      },
    );
    _backendInteractiveResizeSubscription = container.listen(
      xdgToplevelStatesProvider(viewId)
          .select((v) => v.interactiveResizeRequested),
      (_, next) {
        _backendInteractiveResizeActiveEdge = next.edge;
        widget.entry.backendInteractiveResizeEdge = next.edge;
        _backendResizeStartSize = widget.entry.windowRect.size;
        _backendResizeAccumDelta = Offset.zero;
        _backendInteractiveMoveActive = false;
      },
    );
    _backendInteractionListenersAttached = true;
  }

  void _onGlobalPointerEvent(PointerEvent event) {
    if (_backendViewId == null) {
      return;
    }
    if (event is PointerMoveEvent) {
      _lastDragGlobalPosition = event.position;
      if (_backendInteractiveMoveActive) {
        _applyWindowDrag(event.delta, event.position);
        return;
      }
      final edge = _backendInteractiveResizeActiveEdge;
      if (edge != null) {
        if (event.buttons == 0) {
          return;
        }
        final manager = WindowManager.of(context);
        manager?.setClientCursor(
          _cursorForResizeEdge(edge),
          event.position,
        );
        _backendResizeAccumDelta += event.delta;
        final startSize =
            _backendResizeStartSize ?? widget.entry.windowRect.size;
        final sizeDelta =
            _computeResizeSizeDelta(edge, _backendResizeAccumDelta);
        final targetWidth =
            max(widget.entry.minSize.width, startSize.width + sizeDelta.dx);
        final targetHeight =
            max(widget.entry.minSize.height, startSize.height + sizeDelta.dy);
        final viewId = _backendViewId;
        if (viewId != null) {
          final rect = widget.entry.windowRect;
          final clientInsetTop = _backendClientInsetTopFor(widget.entry);
          _queueBackendResizeRequest(
            viewId: viewId,
            width: targetWidth.round(),
            height: targetHeight.round(),
            x: rect.left,
            y: rect.top + clientInsetTop,
          );
        }
      }
      return;
    }

    if (event is PointerUpEvent || event is PointerCancelEvent) {
      final wasMoveActive = _backendInteractiveMoveActive;
      final manager = WindowManager.of(context);
      if (wasMoveActive && _lastDragGlobalPosition != null) {
        _finalizeWindowDrag(_lastDragGlobalPosition!);
      }
      _backendInteractiveMoveActive = false;
      _backendInteractiveResizeActiveEdge = null;
      widget.entry.backendInteractiveResizeEdge = null;
      _backendResizeStartSize = null;
      _backendResizeAccumDelta = Offset.zero;
      _cancelBackendResizeDispatchTimer();
      _pendingBackendResizeRequest = null;
      _rawLeft = _rawTop = _rawRight = _rawBottom = null;
      _dragRawLeft = _dragRawTop = null;
      manager?.endClientCursor();
    }
  }

  Offset _computeResizeSizeDelta(ResizeEdge edge, Offset delta) {
    final dx = delta.dx;
    final dy = delta.dy;
    switch (edge) {
      case ResizeEdge.topLeft:
        return Offset(-dx, -dy);
      case ResizeEdge.top:
        return Offset(0, -dy);
      case ResizeEdge.topRight:
        return Offset(dx, -dy);
      case ResizeEdge.right:
        return Offset(dx, 0);
      case ResizeEdge.bottomRight:
        return Offset(dx, dy);
      case ResizeEdge.bottom:
        return Offset(0, dy);
      case ResizeEdge.bottomLeft:
        return Offset(-dx, dy);
      case ResizeEdge.left:
        return Offset(-dx, 0);
    }
  }

  void _notifyBackendResize(BuildContext context) {
    final viewId = _backendViewId;
    if (viewId == null) {
      return;
    }
    final rect = widget.entry.windowRect;
    final clientInsetTop = _backendClientInsetTopFor(widget.entry);
    _queueBackendResizeRequest(
      viewId: viewId,
      width: rect.width.round(),
      height: rect.height.round(),
      x: rect.left,
      y: rect.top + clientInsetTop,
    );
  }

  void _queueBackendResizeRequest({
    required int viewId,
    required int width,
    required int height,
    required double x,
    required double y,
  }) {
    final request = _BackendResizeRequest(
      viewId: viewId,
      width: width,
      height: height,
      x: x,
      y: y,
    );

    if (_pendingBackendResizeRequest == request) {
      return;
    }

    _pendingBackendResizeRequest = request;
    if (_shouldThrottleXwaylandInteractiveResize(viewId)) {
      _scheduleThrottledBackendResizeFlush();
      return;
    }
    _flushBackendResizeRequest();
  }

  bool _shouldThrottleXwaylandInteractiveResize(int viewId) {
    if (_backendInteractiveResizeActiveEdge == null) {
      return false;
    }
    final container = _backendContainer(context);
    if (container == null) {
      return false;
    }
    return container.read(platformApiProvider.notifier).isXwaylandView(viewId);
  }

  void _scheduleThrottledBackendResizeFlush() {
    if (_backendResizeDispatchTimer != null) {
      return;
    }
    const interval = Duration(milliseconds: 16);
    final nowMicros = DateTime.now().microsecondsSinceEpoch;
    final elapsedMicros = nowMicros - _lastBackendResizeDispatchMicros;
    final elapsed =
        Duration(microseconds: elapsedMicros < 0 ? 0 : elapsedMicros);
    final delay = elapsed >= interval ? Duration.zero : interval - elapsed;

    _backendResizeDispatchTimer = Timer(delay, () {
      _backendResizeDispatchTimer = null;
      if (!mounted) {
        return;
      }
      _flushBackendResizeRequest();
      if (_pendingBackendResizeRequest != null) {
        _scheduleThrottledBackendResizeFlush();
      }
    });
  }

  void _flushBackendResizeRequest() {
    if (_backendResizeRequestInFlight) {
      return;
    }
    final request = _pendingBackendResizeRequest;
    if (request == null) {
      return;
    }
    final container = _backendContainer(context);
    if (container == null) {
      _pendingBackendResizeRequest = null;
      return;
    }

    _pendingBackendResizeRequest = null;
    _backendResizeRequestInFlight = true;
    _lastBackendResizeDispatchMicros = DateTime.now().microsecondsSinceEpoch;
    final api = container.read(platformApiProvider.notifier);
    unawaited(
      api
          .resizeWindow(
        request.viewId,
        request.width,
        request.height,
        x: request.x,
        y: request.y,
      )
          .whenComplete(() {
        _backendResizeRequestInFlight = false;
        if (!mounted) {
          return;
        }
        _flushBackendResizeRequest();
      }),
    );
  }

  void _requestBackendInteractiveMove(BuildContext context) {
    final viewId = _backendViewId;
    if (viewId == null) {
      return;
    }
    final container = _backendContainer(context);
    if (container == null) {
      return;
    }
    container
        .read(xdgToplevelStatesProvider(viewId).notifier)
        .requestInteractiveMove();
  }

  void _requestBackendInteractiveResize(BuildContext context, ResizeEdge edge) {
    final viewId = _backendViewId;
    if (viewId == null) {
      return;
    }
    if (_interactiveResizeRequestedForEdge == edge) {
      return;
    }
    final container = _backendContainer(context);
    if (container == null) {
      return;
    }
    _interactiveResizeRequestedForEdge = edge;
    container
        .read(xdgToplevelStatesProvider(viewId).notifier)
        .requestInteractiveResize(edge);
  }

  void _requestBackendMaximize(BuildContext context, bool value) {
    final viewId = _backendViewId;
    if (viewId == null) {
      return;
    }
    final container = _backendContainer(context);
    if (container == null) {
      return;
    }
    if (value) {
      WindowManager.of(context)?.prepareBackendMaximizeForWindow(widget.entry);
    }
    container
        .read(xdgToplevelStatesProvider(viewId).notifier)
        .requestMaximize(value);
  }

  Widget _buildBackendViewportContent(WindowEntry entry) {
    final viewId = _backendViewId;
    if (viewId == null) {
      return entry.content;
    }

    return Consumer(
      child: entry.content,
      builder: (BuildContext context, WidgetRef ref, Widget? child) {
        final rect = _backendViewportRectFromRef(ref, entry, viewId);
        if (rect.size.isEmpty ||
            !rect.left.isFinite ||
            !rect.top.isFinite ||
            !rect.width.isFinite ||
            !rect.height.isFinite) {
          return child!;
        }
        return RectOverflowBox(
          rect: rect,
          child: child!,
        );
      },
    );
  }

  Rect _backendViewportRectFromRef(
      WidgetRef ref, WindowEntry entry, int viewId) {
    return ref.watch(xdgSurfaceStatesProvider(viewId)).visibleBounds;
  }

  @override
  Widget build(BuildContext context) {
    _attachBackendInteractionListenersIfNeeded();
    return provider_pkg.ChangeNotifierProvider<WindowEntry>.value(
      value: widget.entry,
      builder: (context, child) {
        final entry = provider_pkg.Provider.of<WindowEntry>(context);
        final hierarchy =
            provider_pkg.Provider.of<WindowHierarchyState>(context);

        if (entry.minimized) return const SizedBox.shrink();

        final manager = WindowManager.of(context);
        final windowRect =
            manager?.targetRectForWindow(entry) ?? entry.windowRect;
        final docked = entry.maximized || entry.windowDock != WindowDock.normal;
        final isDecorated = entry.chromeMode == WindowChromeMode.decorated;
        final isBackendWindow = entry.backendViewId != null;
        final isBackendDecorated = isBackendWindow && isDecorated;
        final showResizeHandles =
            !docked && entry.allowResize && (isDecorated || isBackendWindow);

        final alignedWindowRect = _alignRect(windowRect);
        final backendDecoratedHeightOffset =
            isBackendDecorated && entry.usesToolbar
                ? _measuredToolbarHeight
                : 0.0;

        final windowContentChild = Listener(
          onPointerDown: (_) {
            final manager = WindowManager.of(context);
            if (manager != null) {
              manager.requestFocus(entry);
            } else {
              hierarchy.requestWindowFocus(entry);
            }
            setState(() {});
          },
          child: _buildContainer(
            isDecorated: isDecorated,
            hasBorder: isDecorated && !docked,
            isFocused: hierarchy.entriesByFocus.isNotEmpty &&
                hierarchy.entriesByFocus.last == entry,
            child: Column(
              children: [
                Visibility(
                  visible: entry.usesToolbar,
                  child: KeyedSubtree(
                    key: _toolbarMeasureKey,
                    child: TitlebarDragCallbacks(
                      onDragStart: _onTitlebarDragStart,
                      onDrag: _onTitlebarDrag,
                      onDragEnd: _onTitlebarDragEnd,
                      child: entry.toolbar ?? const SizedBox.shrink(),
                    ),
                  ),
                ),
                if (isBackendDecorated)
                  RepaintBoundary(
                    key: entry.repaintBoundaryKey,
                    child: ClipRect(
                      child: _buildBackendViewportContent(entry),
                    ),
                  )
                else
                  Expanded(
                    child: RepaintBoundary(
                      key: entry.repaintBoundaryKey,
                      child: isDecorated
                          ? ClipRect(
                              child: _buildBackendViewportContent(entry),
                            )
                          : _buildBackendViewportContent(entry),
                    ),
                  ),
              ],
            ),
          ),
        );
        if (isBackendDecorated && entry.usesToolbar) {
          _scheduleToolbarMeasure();
        }

        final resizeHandles = Visibility(
          visible: showResizeHandles,
          child: WindowResizeGestureDetector(
            borderThickness: _resizingSpacing,
            listeners: _resizeListeners,
            dragWithCursor: _onResizePanWithCursor,
            onPanEnd: (details) {
              if (isBackendWindow) {
                _interactiveResizeRequestedForEdge = null;
                _backendInteractiveResizeActiveEdge = null;
                entry.backendInteractiveResizeEdge = null;
                _cancelBackendResizeDispatchTimer();
                _pendingBackendResizeRequest = null;
                _rawLeft = _rawTop = _rawRight = _rawBottom = null;
                manager?.endClientCursor();
                _updateMonitorFromRect(entry, manager);
                return;
              }
              entry.windowRect = Rect.fromLTWH(
                entry.windowRect.left,
                entry.windowRect.top,
                max(entry.minSize.width, entry.windowRect.width),
                max(entry.minSize.height, entry.windowRect.height),
              );
              _notifyBackendResize(context);
              _interactiveResizeRequestedForEdge = null;
              _rawLeft = _rawTop = _rawRight = _rawBottom = null;
              manager?.endClientCursor();
              _updateMonitorFromRect(entry, manager);
            },
          ),
        );

        final contentPositioned = (isBackendWindow && !isDecorated)
            ? Positioned(
                left: alignedWindowRect.left,
                top: alignedWindowRect.top,
                child: windowContentChild,
              )
            : isBackendDecorated
                ? Positioned(
                    left: alignedWindowRect.left,
                    top: alignedWindowRect.top,
                    child: SizedBox(
                      width: alignedWindowRect.width,
                      child: windowContentChild,
                    ),
                  )
                : Positioned.fromRect(
                    rect: alignedWindowRect,
                    child: windowContentChild,
                  );
        final handleBaseRect = isBackendDecorated
            ? Rect.fromLTWH(
                alignedWindowRect.left,
                alignedWindowRect.top,
                alignedWindowRect.width,
                alignedWindowRect.height + backendDecoratedHeightOffset,
              )
            : alignedWindowRect;
        final handleRect = handleBaseRect.inflate(_resizingSpacing);

        return Positioned.fill(
          child: Stack(
            clipBehavior: Clip.none,
            children: [
              contentPositioned,
              Positioned.fromRect(
                rect: handleRect,
                child: resizeHandles,
              ),
            ],
          ),
        );
      },
    );
  }

  Rect _alignRect(Rect rect) {
    final left = rect.left.isFinite ? rect.left : 0;
    final top = rect.top.isFinite ? rect.top : 0;
    final width = rect.width.isFinite ? max(1.0, rect.width) : 1.0;
    final height = rect.height.isFinite ? max(1.0, rect.height) : 1.0;
    return Rect.fromLTWH(
      left.roundToDouble(),
      top.roundToDouble(),
      width.roundToDouble(),
      height.roundToDouble(),
    );
  }

  Widget _buildContainer({
    required bool isDecorated,
    bool hasBorder = false,
    bool isFocused = false,
    required Widget child,
  }) {
    // CSD windows should not be wrapped/clipped by server chrome widgets.
    if (!isDecorated) {
      return KeyedSubtree(
        key: _mainContainerKey,
        child: child,
      );
    }

    // SSD windows always use the same container type, avoiding gesture breaks
    // when toggling docked/maximized states.
    return DualBorderOutlinedContainer(
      key: _mainContainerKey,
      hasBorder: hasBorder,
      borderRadius:
          hasBorder ? Theme.of(context).borderRadiusLg : BorderRadius.zero,
      boxShadow: hasBorder && isFocused
          ? [
              BoxShadow(
                color: Colors.black.withAlpha(75),
                blurRadius: 20,
                spreadRadius: 3,
                offset: const Offset(0, 10),
              ),
            ]
          : null,
      clipBehavior: Clip.antiAlias,
      child: child,
    );
  }

  void _onTitlebarDragStart(DragDownDetails details) {
    _dragRawLeft = _dragRawTop = null;
    final manager = WindowManager.of(context);
    if (manager != null) {
      manager.requestFocus(widget.entry);
    } else {
      provider_pkg.Provider.of<WindowHierarchyState>(context, listen: false)
          .requestWindowFocus(widget.entry);
    }
    _requestBackendInteractiveMove(context);
  }

  void _onTitlebarDrag(DragUpdateDetails details) {
    _lastDragGlobalPosition = details.globalPosition;
    if (_backendViewId != null) {
      return;
    }
    _applyWindowDrag(details.delta, details.globalPosition);
  }

  void _applyWindowDrag(Offset delta, Offset globalPosition) {
    final entry = widget.entry;
    final hierarchy =
        provider_pkg.Provider.of<WindowHierarchyState>(context, listen: false);
    final manager = WindowManager.of(context);
    final docked = entry.maximized || entry.windowDock != WindowDock.normal;
    final restoreRect = entry.restoreRectAfterMaximize;
    final dockRestoreRect = entry.restoreRectAfterDock;
    final dragWindowSize = (entry.maximized && restoreRect != null)
        ? restoreRect.size
        : (entry.windowDock != WindowDock.normal && dockRestoreRect != null)
            ? dockRestoreRect.size
            : entry.windowRect.size;

    if (manager != null) {
      final newMonitor = manager.getMonitorAtPosition(globalPosition);
      if (newMonitor != null && newMonitor.id != entry.monitorId) {
        entry.monitorId = newMonitor.id;
      }
      manager.setClientCursor(
        SystemMouseCursors.grabbing,
        globalPosition,
      );
    }

    Rect base;
    if (docked) {
      final dockedRect =
          manager?.targetRectForWindow(entry) ?? entry.windowRect;
      final relativeX =
          (globalPosition.dx - dockedRect.left) / dockedRect.width;
      final newLeft = globalPosition.dx - (dragWindowSize.width * relativeX);

      base = Rect.fromLTWH(
        newLeft,
        dockedRect.top,
        dragWindowSize.width,
        dragWindowSize.height,
      );
      base = base.translate(delta.dx, delta.dy);
      _dragRawLeft = base.left;
      _dragRawTop = base.top;
    } else {
      _dragRawLeft ??= entry.windowRect.left;
      _dragRawTop ??= entry.windowRect.top;
      _dragRawLeft = _dragRawLeft! + delta.dx;
      _dragRawTop = _dragRawTop! + delta.dy;
      base = Rect.fromLTWH(
        _dragRawLeft!,
        _dragRawTop!,
        entry.windowRect.width,
        entry.windowRect.height,
      );
    }

    if (manager != null) {
      manager.requestFocus(entry);
    } else {
      hierarchy.requestWindowFocus(entry);
    }
    if (entry.maximized) {
      _requestBackendMaximize(context, false);
    }
    entry.maximized = false;
    if (entry.windowDock != WindowDock.normal) {
      entry.restoreRectAfterDock = null;
      entry.restoreMonitorIdAfterDock = null;
    }
    entry.windowDock = WindowDock.normal;

    entry.windowRect = base;
    if (entry.backendViewId != null) {
      manager?.syncBackendWindowGeometry(entry, force: true);
    }
    setState(() {});
  }

  void _onTitlebarDragEnd(DragEndDetails details) {
    _dragRawLeft = _dragRawTop = null;
    if (_backendViewId != null) {
      return;
    }
    final pos = _lastDragGlobalPosition;
    if (pos == null) {
      return;
    }
    _finalizeWindowDrag(pos);
  }

  void _finalizeWindowDrag(Offset globalPosition) {
    final entry = widget.entry;
    final manager = WindowManager.of(context);
    if (manager == null) return;

    final monitor = manager.getMonitorById(entry.monitorId ?? '');
    if (monitor == null) return;

    if (entry.windowRect.top < monitor.usableBounds.top) {
      entry.windowRect = entry.windowRect.translate(
        0,
        monitor.usableBounds.top - entry.windowRect.top,
      );
    }

    final regionKey = manager.getRegionKey(monitor.id);
    if (regionKey == null) return;

    manager.endClientCursor();

    final RenderBox? regionBox =
        regionKey.currentContext?.findRenderObject() as RenderBox?;
    if (regionBox == null) return;

    final localPosition = regionBox.globalToLocal(globalPosition);
    final localUsableWidth = monitor.usableBounds.width;

    if ((localPosition.dy <= _dockThickness && localPosition.dx <= 50) ||
        (localPosition.dy <= 50 && localPosition.dx <= _dockThickness)) {
      _captureDockRestore(entry);
      entry.windowDock = WindowDock.topLeft;
      manager.syncBackendWindowGeometry(entry, force: true);
      return;
    }
    if ((localPosition.dy <= _dockThickness && localPosition.dx >= localUsableWidth - 50) ||
        (localPosition.dy <= 50 && localPosition.dx >= localUsableWidth - _dockThickness)) {
      _captureDockRestore(entry);
      entry.windowDock = WindowDock.topRight;
      manager.syncBackendWindowGeometry(entry, force: true);
      return;
    }
    if (localPosition.dy <= _dockThickness) {
      _requestBackendMaximize(context, true);
      entry.maximized = true;
      return;
    }
    if (localPosition.dx <= _dockThickness) {
      _captureDockRestore(entry);
      entry.windowDock = WindowDock.left;
      manager.syncBackendWindowGeometry(entry, force: true);
      return;
    }
    if (localPosition.dx >= localUsableWidth - _dockThickness) {
      _captureDockRestore(entry);
      entry.windowDock = WindowDock.right;
      manager.syncBackendWindowGeometry(entry, force: true);
      return;
    }
  }

  void _captureDockRestore(WindowEntry entry) {
    if (entry.windowDock != WindowDock.normal || entry.maximized) {
      return;
    }
    entry.restoreRectAfterDock = entry.windowRect;
    entry.restoreMonitorIdAfterDock = entry.monitorId;
  }

  SystemMouseCursor _cursorForResizeEdge(ResizeEdge edge) {
    switch (edge) {
      case ResizeEdge.topLeft:
      case ResizeEdge.bottomRight:
        return SystemMouseCursors.resizeUpLeftDownRight;
      case ResizeEdge.topRight:
      case ResizeEdge.bottomLeft:
        return SystemMouseCursors.resizeUpRightDownLeft;
      case ResizeEdge.left:
      case ResizeEdge.right:
        return SystemMouseCursors.resizeLeftRight;
      case ResizeEdge.top:
      case ResizeEdge.bottom:
        return SystemMouseCursors.resizeUpDown;
    }
  }

  Map<Alignment, GestureDragUpdateCallback> get _resizeListeners => {
        Alignment.topLeft: (details) =>
            _onResizePanUpdate(details, top: true, left: true),
        Alignment.topCenter: (details) =>
            _onResizePanUpdate(details, top: true),
        Alignment.topRight: (details) =>
            _onResizePanUpdate(details, top: true, right: true),
        Alignment.centerLeft: (details) =>
            _onResizePanUpdate(details, left: true),
        Alignment.centerRight: (details) =>
            _onResizePanUpdate(details, right: true),
        Alignment.bottomLeft: (details) =>
            _onResizePanUpdate(details, bottom: true, left: true),
        Alignment.bottomCenter: (details) =>
            _onResizePanUpdate(details, bottom: true),
        Alignment.bottomRight: (details) =>
            _onResizePanUpdate(details, bottom: true, right: true),
      };

  void _onResizePanUpdate(
    DragUpdateDetails details, {
    bool left = false,
    bool top = false,
    bool right = false,
    bool bottom = false,
  }) {
    final manager = WindowManager.of(context);
    if (manager != null) {
      manager.requestFocus(widget.entry);
    } else {
      provider_pkg.Provider.of<WindowHierarchyState>(context, listen: false)
          .requestWindowFocus(widget.entry);
    }

    final edge = _edgeFromSides(
      left: left,
      top: top,
      right: right,
      bottom: bottom,
    );
    if (edge != null) {
      _requestBackendInteractiveResize(context, edge);
    }

    if (widget.entry.backendViewId != null) {
      return;
    }

    final minWidth = widget.entry.minSize.width;
    final minHeight = widget.entry.minSize.height;
    final current = widget.entry.windowRect;

    // Initialize accumulators from current rect on first call of a drag gesture.
    _rawLeft ??= current.left;
    _rawTop ??= current.top;
    _rawRight ??= current.right;
    _rawBottom ??= current.bottom;

    // Accumulate raw (unclamped) deltas.
    if (left) _rawLeft = _rawLeft! + details.delta.dx;
    if (top) _rawTop = _rawTop! + details.delta.dy;
    if (right) _rawRight = _rawRight! + details.delta.dx;
    if (bottom) _rawBottom = _rawBottom! + details.delta.dy;

    // Derive clamped rect: left/top are clamped against right/bottom minus min size.
    final clampedLeft =
        left ? min(_rawLeft!, _rawRight! - minWidth) : current.left;
    final clampedTop =
        top ? min(_rawTop!, _rawBottom! - minHeight) : current.top;
    final clampedRight = right ? _rawRight! : current.right;
    final clampedBottom = bottom ? _rawBottom! : current.bottom;

    widget.entry.windowRect = Rect.fromLTRB(
      clampedLeft,
      clampedTop,
      clampedRight,
      clampedBottom,
    );
    _notifyBackendResize(context);

    setState(() {});
  }

  ResizeEdge? _edgeFromSides({
    required bool left,
    required bool top,
    required bool right,
    required bool bottom,
  }) {
    if (top && left) return ResizeEdge.topLeft;
    if (top && right) return ResizeEdge.topRight;
    if (bottom && left) return ResizeEdge.bottomLeft;
    if (bottom && right) return ResizeEdge.bottomRight;
    if (top) return ResizeEdge.top;
    if (bottom) return ResizeEdge.bottom;
    if (left) return ResizeEdge.left;
    if (right) return ResizeEdge.right;
    return null;
  }

  void _onResizePanWithCursor(
    SystemMouseCursor cursor,
    DragUpdateDetails details,
  ) {
    final manager = WindowManager.of(context);
    manager?.setClientCursor(cursor, details.globalPosition);
  }

  void _updateMonitorFromRect(WindowEntry entry, WindowManagerState? manager) {
    if (manager == null) return;
    final centre = entry.windowRect.center;
    final monitor = manager.getMonitorAtPosition(centre);
    if (monitor != null && monitor.id != entry.monitorId) {
      entry.monitorId = monitor.id;
    }
  }
}

class _BackendResizeRequest {
  final int viewId;
  final int width;
  final int height;
  final double x;
  final double y;

  const _BackendResizeRequest({
    required this.viewId,
    required this.width,
    required this.height,
    required this.x,
    required this.y,
  });

  @override
  bool operator ==(Object other) {
    return other is _BackendResizeRequest &&
        other.viewId == viewId &&
        other.width == width &&
        other.height == height &&
        other.x == x &&
        other.y == y;
  }

  @override
  int get hashCode => Object.hash(viewId, width, height, x, y);
}
