import 'dart:ui';

/// A window inside a repaint boundary must invalidate the stack's paint masks
/// when its geometry or opacity changes, even if other windows stay retained.
abstract interface class WindowOcclusionOwner {
  void invalidateOcclusion();
}

List<Rect> subtractOpaqueRect(Rect source, Rect opaque) {
  if (source.isEmpty) return const [];
  if (opaque.isEmpty || !source.overlaps(opaque)) return [source];
  final cut = source.intersect(opaque);
  return [
    Rect.fromLTRB(source.left, source.top, source.right, cut.top),
    Rect.fromLTRB(source.left, cut.bottom, source.right, source.bottom),
    Rect.fromLTRB(source.left, cut.top, cut.left, cut.bottom),
    Rect.fromLTRB(cut.right, cut.top, source.right, cut.bottom),
  ].where((rect) => !rect.isEmpty).toList();
}

/// Disjoint rectangles of pixels still required from lower windows. Complexity
/// is bounded: a fragmented region falls back to overdraw, never missing pixels.
class WindowPaintRegion {
  WindowPaintRegion(this.bounds) : rects = [bounds];
  final Rect bounds;
  List<Rect> rects;
  static const maxRects = 128;

  void subtract(Rect opaque) {
    rects = [for (final rect in rects) ...subtractOpaqueRect(rect, opaque)];
    _limit();
  }

  void add(Rect area) {
    if (!area.isFinite || !area.overlaps(bounds)) return;
    var additions = [area.intersect(bounds)];
    for (final rect in rects) {
      additions = [
        for (final addition in additions) ...subtractOpaqueRect(addition, rect)
      ];
      if (additions.length + rects.length > maxRects) {
        rects = [bounds];
        return;
      }
    }
    rects = [...rects, ...additions];
    _limit();
  }

  void _limit() {
    if (rects.length > maxRects) rects = [bounds];
  }

  void requireBackdrop(List<Rect> visible, List<Rect> filters, double outset) {
    for (final filter in filters) {
      for (final rect in visible) {
        if (rect.overlaps(filter)) add(rect.intersect(filter).inflate(outset));
      }
    }
  }
}
