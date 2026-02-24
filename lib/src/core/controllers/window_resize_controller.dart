import 'dart:math';

import 'package:flutter/material.dart';
import 'package:jdwm/src/core/models/resize.dart';

class WindowResizeSnapshot {
  final bool resizing;
  final ResizeEdge? resizeEdge;
  final Size startSize;
  final Size wantedSize;
  final Offset delta;

  const WindowResizeSnapshot({
    required this.resizing,
    required this.resizeEdge,
    required this.startSize,
    required this.wantedSize,
    required this.delta,
  });
}

class WindowResizeUpdate {
  final WindowResizeSnapshot state;
  final Size? resizeTo;

  const WindowResizeUpdate({
    required this.state,
    this.resizeTo,
  });
}

class WindowResizeController {
  const WindowResizeController();

  WindowResizeSnapshot initial() {
    return const WindowResizeSnapshot(
      resizing: false,
      resizeEdge: null,
      startSize: Size.zero,
      wantedSize: Size.zero,
      delta: Offset.zero,
    );
  }

  WindowResizeSnapshot startPotentialResize(WindowResizeSnapshot state) {
    return WindowResizeSnapshot(
      resizing: false,
      resizeEdge: state.resizeEdge,
      startSize: state.startSize,
      wantedSize: state.wantedSize,
      delta: Offset.zero,
    );
  }

  WindowResizeSnapshot startResize(
    WindowResizeSnapshot state,
    ResizeEdge edge,
    Size size,
  ) {
    if (state.resizing) {
      return state;
    }

    Size wantedSize = state.wantedSize;
    if (state.delta != Offset.zero) {
      final resized = _computeResizeOffset(edge, state.delta);
      wantedSize = size + resized;
    }

    return WindowResizeSnapshot(
      resizing: true,
      resizeEdge: edge,
      startSize: size,
      wantedSize: wantedSize,
      delta: state.delta,
    );
  }

  WindowResizeUpdate resize(WindowResizeSnapshot state, Offset delta) {
    final nextDelta = state.delta + delta;
    if (!state.resizing) {
      return WindowResizeUpdate(
        state: WindowResizeSnapshot(
          resizing: state.resizing,
          resizeEdge: state.resizeEdge,
          startSize: state.startSize,
          wantedSize: state.wantedSize,
          delta: nextDelta,
        ),
      );
    }

    final resized = _computeResizeOffset(state.resizeEdge, nextDelta);
    final wantedSize = state.startSize + resized;
    final width = max(1, wantedSize.width.toInt());
    final height = max(1, wantedSize.height.toInt());

    return WindowResizeUpdate(
      state: WindowResizeSnapshot(
        resizing: state.resizing,
        resizeEdge: state.resizeEdge,
        startSize: state.startSize,
        wantedSize: wantedSize,
        delta: nextDelta,
      ),
      resizeTo: Size(width.toDouble(), height.toDouble()),
    );
  }

  WindowResizeSnapshot endResize(WindowResizeSnapshot state) {
    if (!state.resizing) {
      return state;
    }
    return WindowResizeSnapshot(
      resizing: false,
      resizeEdge: null,
      startSize: state.startSize,
      wantedSize: state.wantedSize,
      delta: Offset.zero,
    );
  }

  WindowResizeUpdate cancelResize(WindowResizeSnapshot state) {
    if (!state.resizing) {
      return WindowResizeUpdate(state: state);
    }

    final width = max(1, state.startSize.width.toInt());
    final height = max(1, state.startSize.height.toInt());
    final wantedSize = Size(width.toDouble(), height.toDouble());

    return WindowResizeUpdate(
      state: WindowResizeSnapshot(
        resizing: false,
        resizeEdge: null,
        startSize: state.startSize,
        wantedSize: wantedSize,
        delta: Offset.zero,
      ),
      resizeTo: wantedSize,
    );
  }

  Offset computeWindowOffset(
    ResizeEdge? resizeEdge,
    Size oldSize,
    Size newSize,
  ) {
    final offset = (newSize - oldSize) as Offset;
    final dx = offset.dx;
    final dy = offset.dy;

    switch (resizeEdge) {
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
      case null:
        return Offset.zero;
    }
  }

  Offset _computeResizeOffset(ResizeEdge? edge, Offset delta) {
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
      case null:
        return Offset(dx, dy);
    }
  }
}
