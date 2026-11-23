import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../util/backend_surface_geometry.dart';
import 'surface_state.dart';
import 'xdg_surface_state.dart';

final managedSurfaceRectProvider =
    Provider.autoDispose.family<Rect, int>((ref, viewId) {
  final bounds = ref
      .watch(xdgSurfaceStatesProvider(viewId).select((s) => s.visibleBounds));
  final size =
      ref.watch(surfaceStatesProvider(viewId).select((s) => s.surfaceSize));
  return backendManagedSurfaceRect(surfaceSize: size, visibleBounds: bounds);
});
