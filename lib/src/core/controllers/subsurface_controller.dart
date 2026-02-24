import 'package:flutter/material.dart';

class SubsurfaceSnapshot {
  final Offset position;
  final bool mapped;
  final Key widgetKey;

  const SubsurfaceSnapshot({
    required this.position,
    required this.mapped,
    required this.widgetKey,
  });
}

class SubsurfaceController {
  const SubsurfaceController();

  SubsurfaceSnapshot initial({required Key widgetKey}) {
    return SubsurfaceSnapshot(
      position: Offset.zero,
      mapped: false,
      widgetKey: widgetKey,
    );
  }

  SubsurfaceSnapshot commit(SubsurfaceSnapshot state, {required Offset position}) {
    return SubsurfaceSnapshot(
      position: position,
      mapped: state.mapped,
      widgetKey: state.widgetKey,
    );
  }

  SubsurfaceSnapshot map(SubsurfaceSnapshot state, bool value) {
    return SubsurfaceSnapshot(
      position: state.position,
      mapped: value,
      widgetKey: state.widgetKey,
    );
  }
}
