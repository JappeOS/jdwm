import 'package:flutter/material.dart';
import 'package:jdwm/src/core/models/surface_role.dart';

class SurfaceSnapshot {
  final SurfaceRole role;
  final int viewId;
  final int textureId;
  final Offset surfacePosition;
  final Size surfaceSize;
  final double scale;
  final GlobalKey widgetKey;
  final GlobalKey textureKey;
  final List<int> subsurfacesBelow;
  final List<int> subsurfacesAbove;
  final Rect inputRegion;

  const SurfaceSnapshot({
    required this.role,
    required this.viewId,
    required this.textureId,
    required this.surfacePosition,
    required this.surfaceSize,
    required this.scale,
    required this.widgetKey,
    required this.textureKey,
    required this.subsurfacesBelow,
    required this.subsurfacesAbove,
    required this.inputRegion,
  });
}

class SurfaceCommitData {
  final SurfaceRole role;
  final int textureId;
  final Offset surfacePosition;
  final Size surfaceSize;
  final double scale;
  final List<int> subsurfacesBelow;
  final List<int> subsurfacesAbove;
  final Rect inputRegion;

  const SurfaceCommitData({
    required this.role,
    required this.textureId,
    required this.surfacePosition,
    required this.surfaceSize,
    required this.scale,
    required this.subsurfacesBelow,
    required this.subsurfacesAbove,
    required this.inputRegion,
  });
}

class SurfaceController {
  const SurfaceController();

  SurfaceSnapshot initial({
    required int viewId,
    required SurfaceRole role,
    required GlobalKey widgetKey,
    required GlobalKey textureKey,
  }) {
    return SurfaceSnapshot(
      role: role,
      viewId: viewId,
      textureId: -1,
      surfacePosition: Offset.zero,
      surfaceSize: Size.zero,
      scale: 1,
      widgetKey: widgetKey,
      textureKey: textureKey,
      subsurfacesBelow: const <int>[],
      subsurfacesAbove: const <int>[],
      inputRegion: Rect.zero,
    );
  }

  SurfaceSnapshot commit(SurfaceSnapshot state, SurfaceCommitData data) {
    return SurfaceSnapshot(
      role: data.role,
      viewId: state.viewId,
      textureId: data.textureId,
      surfacePosition: data.surfacePosition,
      surfaceSize: data.surfaceSize,
      scale: data.scale,
      widgetKey: state.widgetKey,
      textureKey: state.textureKey,
      subsurfacesBelow: data.subsurfacesBelow,
      subsurfacesAbove: data.subsurfacesAbove,
      inputRegion: data.inputRegion,
    );
  }
}
