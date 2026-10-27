import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:jdwm/src/util/flutter_frame_trace.dart';
import 'package:jdwm/src/util/window_occlusion.dart';
import 'window_backdrop.dart';

/// Retain stationary window layers, but propagate layout/paint changes to the
/// stack which owns their visibility masks (including ancestor-only moves).
class WindowPaintBoundary extends SingleChildRenderObjectWidget {
  const WindowPaintBoundary({super.key, required super.child});

  @override
  RenderRepaintBoundary createRenderObject(BuildContext context) =>
      _WindowPaintBoundary();
}

class _WindowPaintBoundary extends RenderRepaintBoundary {
  @override
  void markNeedsPaint() {
    super.markNeedsPaint();
    RenderObject? ancestor = parent;
    while (ancestor != null) {
      if (ancestor is WindowOcclusionOwner) {
        (ancestor as WindowOcclusionOwner).invalidateOcclusion();
        return;
      }
      ancestor = ancestor.parent;
    }
  }
}

/// Keeps every window mounted and hit-testable; only its paint is occluded.
class WindowOcclusionStack extends Stack {
  const WindowOcclusionStack(
      {super.key, required this.contentKeys, required super.children})
      : super(clipBehavior: Clip.none);

  final List<GlobalKey?> contentKeys;

  @override
  RenderWindowOcclusionStack createRenderObject(BuildContext context) =>
      RenderWindowOcclusionStack(contentKeys, Directionality.of(context));

  @override
  void updateRenderObject(
      BuildContext context, RenderWindowOcclusionStack renderObject) {
    super.updateRenderObject(context, renderObject);
    renderObject.contentKeys = contentKeys;
  }
}

class RenderWindowOcclusionStack extends RenderStack
    implements WindowOcclusionOwner {
  RenderWindowOcclusionStack(this._contentKeys, TextDirection direction)
      : super(textDirection: direction, clipBehavior: Clip.none);

  List<GlobalKey?> _contentKeys;
  set contentKeys(List<GlobalKey?> value) {
    if (listEquals(value, _contentKeys)) return;
    _contentKeys = value;
    markNeedsPaint();
  }

  @override
  bool get alwaysNeedsCompositing => true;

  @override
  void invalidateOcclusion() => markNeedsPaint();

  RenderWindowBackdrop? _backdrop(int index, RenderBox child) {
    if (index >= _contentKeys.length) return null;
    RenderObject? node =
        _contentKeys[index]?.currentContext?.findRenderObject();
    RenderWindowBackdrop? result;
    while (node != null) {
      if (node is RenderWindowBackdrop) result = node;
      if (result == null &&
          node is! RenderRepaintBoundary &&
          node is! RenderFlex &&
          node is! RenderDecoratedBox &&
          node is! RenderClipRect &&
          node is! RenderClipRRect &&
          node is! RenderClipPath &&
          node is! RenderPadding &&
          node is! RenderConstrainedBox &&
          node is! RenderSemanticsAnnotations) {
        return null;
      }
      // Unknown effects wrapping a window may alter alpha or sample pixels
      // outside its bounds. Such children retain ordinary stack rendering.
      if (result != null &&
          node != result &&
          node is! RenderRepaintBoundary &&
          node is! RenderPointerListener &&
          node is! RenderStack &&
          node is! RenderConstrainedBox) {
        return null;
      }
      if (node == child) return result;
      node = node.parent;
    }
    return null;
  }

  @override
  void paintStack(PaintingContext context, Offset offset) {
    final children = getChildrenAsList();
    final needed = WindowPaintRegion(Offset.zero & size);
    final masks = List<List<Rect>>.generate(children.length, (_) => const []);
    final paintedBounds = List<Rect?>.filled(children.length, null);

    for (var i = children.length - 1; i >= 0; i--) {
      final child = children[i];
      masks[i] = List.of(needed.rects);
      if (child.size.isEmpty) continue;
      final backdrop = _backdrop(i, child);
      final translation = backdrop == null
          ? null
          : MatrixUtils.getAsTranslation(backdrop.getTransformTo(this));
      if (backdrop == null || translation == null) {
        // We cannot infer the input dependencies of arbitrary overlay widgets.
        needed.rects = [needed.bounds];
        continue;
      }
      // JDWM's fixed window shadow, border and resize handles fit within this
      // conservative margin. Do not cull based on content bounds alone.
      paintedBounds[i] =
          (Offset.zero & backdrop.size).inflate(64).shift(translation);
      needed.subtract(backdrop.opaquePaintRect.shift(translation));
      needed.requireBackdrop(
          masks[i],
          backdrop.backdropPaintRects
              .map((rect) => rect.shift(translation))
              .toList(),
          backdrop.backdropOutset);
    }

    for (var i = 0; i < children.length; i++) {
      final child = children[i];
      final bounds = paintedBounds[i] ?? needed.bounds;
      final visible = masks[i]
          .where((rect) => rect.overlaps(bounds))
          .map((rect) => rect.intersect(bounds))
          .toList();
      FlutterFrameTrace.occlusion(bounds.intersect(needed.bounds), visible);
      if (visible.isEmpty) continue;
      final parentData = child.parentData! as StackParentData;
      if (masks[i].length == 1 && masks[i].single == needed.bounds) {
        context.paintChild(child, offset + parentData.offset);
        continue;
      }
      // Disjoint rectangles use non-zero winding, never an even-odd mask.
      final path = Path();
      for (final rect in visible) {
        path.addRect(rect);
      }
      context.pushClipPath(
          true,
          offset,
          needed.bounds,
          path,
          (context, offset) =>
              context.paintChild(child, offset + parentData.offset),
          clipBehavior: Clip.hardEdge);
    }
  }
}
