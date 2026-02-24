import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/core/controllers/window_move_controller.dart';

part 'window_move_provider.freezed.dart';

part 'window_move_provider.g.dart';

final windowMoveControllerProvider = Provider<WindowMoveController>(
  (ref) => const WindowMoveController(),
);

@Riverpod(keepAlive: true)
class WindowMove extends _$WindowMove {
  @override
  WindowMoveState build(int viewId) {
    final controller = ref.read(windowMoveControllerProvider);
    return _fromSnapshot(controller.initial());
  }

  void startPotentialMove() {
    state = _fromSnapshot(
      ref.read(windowMoveControllerProvider).startPotentialMove(_toSnapshot(state)),
    );
  }

  void startMove(Offset position) {
    state = _fromSnapshot(
      ref.read(windowMoveControllerProvider).startMove(_toSnapshot(state), position),
    );
  }

  void move(Offset delta) {
    state = _fromSnapshot(
      ref.read(windowMoveControllerProvider).move(_toSnapshot(state), delta),
    );
  }

  void endMove() {
    state = _fromSnapshot(
      ref.read(windowMoveControllerProvider).endMove(_toSnapshot(state)),
    );
  }

  void cancelMove() {
    state = _fromSnapshot(
      ref.read(windowMoveControllerProvider).cancelMove(_toSnapshot(state)),
    );
  }

  WindowMoveSnapshot _toSnapshot(WindowMoveState s) {
    return WindowMoveSnapshot(
      moving: s.moving,
      startPosition: s.startPosition,
      movedPosition: s.movedPosition,
      delta: s.delta,
    );
  }

  WindowMoveState _fromSnapshot(WindowMoveSnapshot s) {
    return WindowMoveState(
      moving: s.moving,
      startPosition: s.startPosition,
      movedPosition: s.movedPosition,
      delta: s.delta,
    );
  }
}

@freezed
abstract class WindowMoveState with _$WindowMoveState {
  const factory WindowMoveState({
    required bool moving,
    required Offset startPosition,
    required Offset movedPosition,
    required Offset delta,
  }) = _WindowMoveState;
}
