import 'dart:async';

import 'package:flutter/rendering.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/input/mouse_button_tracker.dart';

part 'pointer_focus_manager.g.dart';

@Riverpod(keepAlive: true)
PointerFocusManager pointerFocusManager(Ref ref) => PointerFocusManager(ref);

/// Routes the compositor's mouse seat to the hit-tested client surface.
class PointerFocusManager {
  PointerFocusManager(this._ref);

  final Ref _ref;
  ({int id, RenderBox box})? _hovered;
  Offset _pointerPosition = Offset.zero;
  int? _routedView;
  Offset? _routedPosition;
  ({int id, RenderBox box})? _pressed;
  bool _updatePending = false;
  bool _enterPending = false;

  void enterSurface(int viewId, RenderBox box, Offset globalPosition) {
    _hovered = (id: viewId, box: box);
    // A remapped surface can reuse its view ID after native focus was cleared.
    _enterPending = true;
    _pointerPosition = globalPosition;
    _scheduleUpdate();
  }

  void exitSurface(int viewId) {
    if (_hovered?.id != viewId) return;

    _hovered = null;
    _enterPending = false;
    _scheduleUpdate();
  }

  void removeSurface(int viewId) {
    exitSurface(viewId);
    releaseButtons(viewId);
  }

  void _scheduleUpdate() {
    if (_updatePending) return;
    _updatePending = true;

    // MouseTracker delivers exits then enters in one update. Resolve them
    // together so moving onto a popup transfers focus directly to it.
    scheduleMicrotask(() {
      _updatePending = false;
      final next = _hovered;
      if (!_ref.mounted || _pressed != null) return;
      if (next != null) {
        final position = next.box.globalToLocal(_pointerPosition);
        if (_enterPending ||
            next.id != _routedView ||
            position != _routedPosition) {
          _move(next.id, position);
        }
      } else if (_routedView != null) {
        _routedView = null;
        unawaited(_ref.read(platformApiProvider.notifier).pointerExitsView());
      }
    });
  }

  void hover(int viewId, Offset position, Offset globalPosition) {
    _pointerPosition = globalPosition;
    if (_pressed == null) _move(viewId, position);
  }

  void _move(int viewId, Offset position) {
    if (_hovered?.id == viewId) _enterPending = false;
    _routedView = viewId;
    _routedPosition = position;
    unawaited(_ref
        .read(platformApiProvider.notifier)
        .pointerHoversView(viewId, position));
  }

  void press(int viewId, RenderBox box, Offset globalPosition, int buttons) {
    _pressed = (id: viewId, box: box);
    _pointerPosition = globalPosition;
    _move(viewId, box.globalToLocal(globalPosition));
    _sendButtons(buttons);
  }

  void drag(int viewId, Offset globalPosition, int buttons) {
    final pressed = _pressed;
    if (pressed == null || pressed.id != viewId) return;
    _pointerPosition = globalPosition;
    _sendButtons(buttons);
    // The gesture route retains the transform from button-down. The client
    // surface can move under that route while the compositor drags its window.
    _move(viewId, pressed.box.globalToLocal(globalPosition));
  }

  void releaseButtons(int viewId, [Offset? globalPosition]) {
    final pressed = _pressed;
    if (pressed == null || pressed.id != viewId) return;

    if (globalPosition != null) {
      _pointerPosition = globalPosition;
      final position = pressed.box.globalToLocal(globalPosition);
      if (position != _routedPosition) _move(viewId, position);
    }

    _sendButtons(0);
    _pressed = null;
    _scheduleUpdate();
    
    if (globalPosition != null) {
      // Release can arrive before the final move or snap has been laid out.
      // Resolve the latest hit target again once MouseTracker has run.
      SchedulerBinding.instance.endOfFrame.then((_) => _scheduleUpdate());
    }
  }

  void _sendButtons(int buttons) {
    final tracker = _ref.read(mouseButtonTrackerProvider);
    for (final event in tracker.trackButtonChanges(buttons)) {
      unawaited(_ref
          .read(platformApiProvider.notifier)
          .sendMouseButtonEventToView(
              event.button, event.state == MouseButtonState.pressed));
    }
  }
}
