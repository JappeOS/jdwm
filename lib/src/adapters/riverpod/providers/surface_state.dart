import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/core/controllers/surface_controller.dart';
import 'package:jdwm/src/core/models/surface_role.dart';
import 'package:jdwm/ui/common/surface.dart';

part 'surface_state.freezed.dart';

part 'surface_state.g.dart';

final surfaceControllerProvider = Provider<SurfaceController>(
  (ref) => const SurfaceController(),
);

@Riverpod(keepAlive: true)
Surface surfaceWidget(Ref ref, int viewId) {
  return Surface(
    key: ref.watch(surfaceStatesProvider(viewId).select((state) => state.widgetKey)),
    viewId: viewId,
  );
}

@freezed
abstract class SurfaceState with _$SurfaceState {
  const factory SurfaceState({
    required SurfaceRole role,
    required int viewId,
    required int textureId,
    required Offset surfacePosition,
    required Size surfaceSize,
    required double scale,
    required GlobalKey widgetKey,
    required GlobalKey textureKey,
    required List<int> subsurfacesBelow,
    required List<int> subsurfacesAbove,
    required Rect inputRegion,
  }) = _SurfaceState;
}

@Riverpod(keepAlive: true)
class SurfaceStates extends _$SurfaceStates {
  @override
  SurfaceState build(int viewId) {
    return _fromSnapshot(
      ref.read(surfaceControllerProvider).initial(
        viewId: viewId,
        role: SurfaceRole.none,
        widgetKey: GlobalKey(),
        textureKey: GlobalKey(),
      ),
    );
  }

  void commit({
    required SurfaceRole role,
    required int textureId,
    required Offset surfacePosition,
    required Size surfaceSize,
    required double scale,
    required List<int> subsurfacesBelow,
    required List<int> subsurfacesAbove,
    required Rect inputRegion,
  }) {
    state = _fromSnapshot(
      ref.read(surfaceControllerProvider).commit(
            _toSnapshot(state),
            SurfaceCommitData(
              role: role,
              textureId: textureId,
              surfacePosition: surfacePosition,
              surfaceSize: surfaceSize,
              scale: scale,
              subsurfacesBelow: subsurfacesBelow,
              subsurfacesAbove: subsurfacesAbove,
              inputRegion: inputRegion,
            ),
          ),
    );
  }

  SurfaceSnapshot _toSnapshot(SurfaceState s) {
    return SurfaceSnapshot(
      role: s.role,
      viewId: viewId,
      textureId: s.textureId,
      surfacePosition: s.surfacePosition,
      surfaceSize: s.surfaceSize,
      scale: s.scale,
      widgetKey: s.widgetKey,
      textureKey: s.textureKey,
      subsurfacesBelow: s.subsurfacesBelow,
      subsurfacesAbove: s.subsurfacesAbove,
      inputRegion: s.inputRegion,
    );
  }

  SurfaceState _fromSnapshot(SurfaceSnapshot s) {
    return SurfaceState(
      role: s.role,
      viewId: s.viewId,
      textureId: s.textureId,
      surfacePosition: s.surfacePosition,
      surfaceSize: s.surfaceSize,
      scale: s.scale,
      widgetKey: s.widgetKey,
      textureKey: s.textureKey,
      subsurfacesBelow: s.subsurfacesBelow,
      subsurfacesAbove: s.subsurfacesAbove,
      inputRegion: s.inputRegion,
    );
  }
}
