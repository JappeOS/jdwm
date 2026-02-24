import 'package:flutter/material.dart';

class WindowMoveSnapshot {
  final bool moving;
  final Offset startPosition;
  final Offset movedPosition;
  final Offset delta;

  const WindowMoveSnapshot({
    required this.moving,
    required this.startPosition,
    required this.movedPosition,
    required this.delta,
  });
}

class WindowMoveController {
  const WindowMoveController();

  WindowMoveSnapshot initial() {
    return const WindowMoveSnapshot(
      moving: false,
      startPosition: Offset.zero,
      movedPosition: Offset.zero,
      delta: Offset.zero,
    );
  }

  WindowMoveSnapshot startPotentialMove(WindowMoveSnapshot state) {
    return WindowMoveSnapshot(
      moving: false,
      startPosition: state.startPosition,
      movedPosition: state.movedPosition,
      delta: Offset.zero,
    );
  }

  WindowMoveSnapshot startMove(WindowMoveSnapshot state, Offset position) {
    if (state.moving) {
      return state;
    }
    final movedPosition =
        state.delta == Offset.zero ? state.movedPosition : position + state.delta;

    return WindowMoveSnapshot(
      moving: true,
      startPosition: position,
      movedPosition: movedPosition,
      delta: state.delta,
    );
  }

  WindowMoveSnapshot move(WindowMoveSnapshot state, Offset delta) {
    final nextDelta = state.delta + delta;
    return WindowMoveSnapshot(
      moving: state.moving,
      startPosition: state.startPosition,
      movedPosition: state.moving ? state.startPosition + nextDelta : state.movedPosition,
      delta: nextDelta,
    );
  }

  WindowMoveSnapshot endMove(WindowMoveSnapshot state) {
    if (!state.moving) {
      return state;
    }
    return WindowMoveSnapshot(
      moving: false,
      startPosition: state.startPosition,
      movedPosition: state.movedPosition,
      delta: Offset.zero,
    );
  }

  WindowMoveSnapshot cancelMove(WindowMoveSnapshot state) {
    if (!state.moving) {
      return state;
    }
    return WindowMoveSnapshot(
      moving: false,
      startPosition: state.startPosition,
      movedPosition: state.startPosition,
      delta: Offset.zero,
    );
  }
}
