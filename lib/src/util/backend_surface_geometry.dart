import 'package:flutter/material.dart';

bool backendUsesSurfaceDestinationSize({
  required Size surfaceSize,
  required Rect visibleBounds,
  required Rect bufferSourceBox,
  required Size bufferSize,
}) {
  // XDG window geometry is already in surface-local coordinates, after
  // buffer scaling and viewport mapping. Those mappings do not replace it.
  return visibleBounds.size.isEmpty && !surfaceSize.isEmpty;
}

Rect backendManagedSurfaceRect({
  required Size surfaceSize,
  required Rect visibleBounds,
  required Rect bufferSourceBox,
  required Size bufferSize,
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
