import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/core/controllers/window_resize_controller.dart';
import 'package:jdwm/src/core/models/resize.dart';

part 'window_resize_provider.freezed.dart';

part 'window_resize_provider.g.dart';

final windowResizeControllerProvider = Provider<WindowResizeController>(
  (ref) => const WindowResizeController(),
);

@Riverpod(keepAlive: true)
class WindowResize extends _$WindowResize {
  @override
  ResizerState build(int viewId) {
    return _fromSnapshot(
      ref.read(windowResizeControllerProvider).initial(),
    );
  }

  void startPotentialResize() {
    state = _fromSnapshot(
      ref.read(windowResizeControllerProvider).startPotentialResize(_toSnapshot(state)),
    );
  }

  void startResize(ResizeEdge edge, Size size) {
    state = _fromSnapshot(
      ref.read(windowResizeControllerProvider).startResize(_toSnapshot(state), edge, size),
    );
  }

  void resize(Offset delta) {
    final update = ref.read(windowResizeControllerProvider).resize(_toSnapshot(state), delta);
    state = _fromSnapshot(update.state);
    final size = update.resizeTo;
    if (size != null) {
      ref.read(platformApiProvider.notifier).resizeWindow(
        viewId,
        size.width.toInt(),
        size.height.toInt(),
      );
    }
  }

  void endResize() {
    state = _fromSnapshot(
      ref.read(windowResizeControllerProvider).endResize(_toSnapshot(state)),
    );
  }

  void cancelResize() {
    final update = ref.read(windowResizeControllerProvider).cancelResize(_toSnapshot(state));
    state = _fromSnapshot(update.state);
    final size = update.resizeTo;
    if (size != null) {
      ref.read(platformApiProvider.notifier).resizeWindow(
        viewId,
        size.width.toInt(),
        size.height.toInt(),
      );
    }
  }

  Offset computeWindowOffset(Size oldSize, Size newSize) {
    return ref
        .read(windowResizeControllerProvider)
        .computeWindowOffset(state.resizeEdge, oldSize, newSize);
  }

  WindowResizeSnapshot _toSnapshot(ResizerState s) {
    return WindowResizeSnapshot(
      resizing: s.resizing,
      resizeEdge: s.resizeEdge,
      startSize: s.startSize,
      wantedSize: s.wantedSize,
      delta: s.delta,
    );
  }

  ResizerState _fromSnapshot(WindowResizeSnapshot s) {
    return ResizerState(
      resizing: s.resizing,
      resizeEdge: s.resizeEdge,
      startSize: s.startSize,
      wantedSize: s.wantedSize,
      delta: s.delta,
    );
  }
}

@freezed
abstract class ResizerState with _$ResizerState {
  const factory ResizerState({
    required bool resizing,
    required ResizeEdge? resizeEdge,
    required Size startSize,
    required Size wantedSize,
    required Offset delta,
  }) = _ResizerState;
}
