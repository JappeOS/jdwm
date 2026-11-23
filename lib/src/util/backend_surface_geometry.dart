import 'package:flutter/material.dart';

Rect backendManagedSurfaceRect({
  required Size surfaceSize,
  required Rect visibleBounds,
}) {
  if (!visibleBounds.size.isEmpty) {
    // Preserve the origin so RectOverflowBox removes client shadow margins
    // instead of clipping the right/bottom edges of the actual window.
    return visibleBounds;
  }
  if (surfaceSize.isEmpty) {
    return Rect.zero;
  }
  return Offset.zero & surfaceSize;
}
