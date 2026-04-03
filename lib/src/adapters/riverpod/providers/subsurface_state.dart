import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/core/controllers/subsurface_controller.dart';
import 'package:jdwm/src/ui/common/subsurface.dart';

part 'subsurface_state.freezed.dart';

part 'subsurface_state.g.dart';

final subsurfaceControllerProvider = Provider<SubsurfaceController>(
  (ref) => const SubsurfaceController(),
);

@Riverpod(keepAlive: true)
Subsurface subsurfaceWidget(Ref ref, int viewId) {
  return Subsurface(
    key: ref.watch(subsurfaceStatesProvider(viewId).select((state) => state.widgetKey)),
    viewId: viewId,
  );

}

@freezed
abstract class SubsurfaceState with _$SubsurfaceState {
  const factory SubsurfaceState({
    required Offset position, // relative to the parent
    required bool mapped,
    required Key widgetKey,
  }) = _SubsurfaceState;
}

@Riverpod(keepAlive: true)
class SubsurfaceStates extends _$SubsurfaceStates {
  @override
  SubsurfaceState build(int viewId) {
    return _fromSnapshot(
      ref.read(subsurfaceControllerProvider).initial(widgetKey: GlobalKey()),
    );
  }

  void commit({required Offset position}) {
    state = _fromSnapshot(
      ref.read(subsurfaceControllerProvider).commit(_toSnapshot(state), position: position),
    );
  }

  void map(bool value) {
    state = _fromSnapshot(
      ref.read(subsurfaceControllerProvider).map(_toSnapshot(state), value),
    );
  }

  SubsurfaceSnapshot _toSnapshot(SubsurfaceState s) {
    return SubsurfaceSnapshot(
      position: s.position,
      mapped: s.mapped,
      widgetKey: s.widgetKey,
    );
  }

  SubsurfaceState _fromSnapshot(SubsurfaceSnapshot s) {
    return SubsurfaceState(
      position: s.position,
      mapped: s.mapped,
      widgetKey: s.widgetKey,
    );
  }
}
