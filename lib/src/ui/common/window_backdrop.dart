import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:jdwm/src/util/flutter_frame_trace.dart';
import 'package:jdwm/src/util/window_occlusion.dart';

/// Disjoint backdrop areas outside a proven opaque rectangle.
List<Rect> exposedBackdropRects(Rect bounds, Rect opaque) {
  if (bounds.isEmpty) return const [];
  if (!opaque.isFinite || opaque.isEmpty || !bounds.overlaps(opaque)) {
    return [bounds];
  }
  final covered = bounds.intersect(opaque);
  return [
    Rect.fromLTRB(bounds.left, bounds.top, bounds.right, covered.top),
    Rect.fromLTRB(bounds.left, covered.bottom, bounds.right, bounds.bottom),
    Rect.fromLTRB(bounds.left, covered.top, covered.left, covered.bottom),
    Rect.fromLTRB(covered.right, covered.top, bounds.right, covered.bottom),
  ].where((rect) => !rect.isEmpty).toList();
}

/// Blurs only the exposed parts of a window's background. Opaque client pixels
/// hide the remaining effect, so filtering them wastes work on every frame.
class WindowBackdrop extends SingleChildRenderObjectWidget {
  final double sigma;
  final BorderRadius radius;
  final GlobalKey contentKey;
  final Rect opaqueRect;

  const WindowBackdrop({
    super.key,
    required this.sigma,
    required this.radius,
    required this.contentKey,
    required this.opaqueRect,
    required super.child,
  });

  @override
  RenderWindowBackdrop createRenderObject(BuildContext context) =>
      RenderWindowBackdrop(
          windowBlurDisabled ? 0 : sigma, radius, contentKey, opaqueRect);

  @override
  void updateRenderObject(
      BuildContext context, RenderWindowBackdrop renderObject) {
    renderObject.configure(
        windowBlurDisabled ? 0 : sigma, radius, contentKey, opaqueRect);
  }
}

class RenderWindowBackdrop extends RenderProxyBox {
  RenderWindowBackdrop(
      this._sigma, this._radius, this._contentKey, this._opaqueRect);

  double _sigma;
  BorderRadius _radius;
  GlobalKey _contentKey;
  Rect _opaqueRect;

  void configure(
      double sigma, BorderRadius radius, GlobalKey key, Rect opaque) {
    _invalidateOcclusion(); // Ancestor positioning can change with the same size.
    if (sigma == _sigma &&
        radius == _radius &&
        key == _contentKey &&
        opaque == _opaqueRect) {
      return;
    }
    if ((sigma > 0) != (_sigma > 0)) markNeedsCompositingBitsUpdate();
    _sigma = sigma;
    _radius = radius;
    _contentKey = key;
    _opaqueRect = opaque;
    markNeedsPaint();
  }

  @override
  bool get alwaysNeedsCompositing => _sigma > 0;

  void _invalidateOcclusion() {
    RenderObject? ancestor = parent;
    while (ancestor != null) {
      if (ancestor is WindowOcclusionOwner) {
        (ancestor as WindowOcclusionOwner).invalidateOcclusion();
        return;
      }
      ancestor = ancestor.parent;
    }
  }

  @override
  void markNeedsPaint() {
    super.markNeedsPaint();
    _invalidateOcclusion();
  }

  @override
  void performLayout() {
    super.performLayout();
    _invalidateOcclusion();
  }

  Rect _contentOpaqueRect() {
    final content = _contentKey.currentContext?.findRenderObject();
    if (_opaqueRect.isEmpty || content is! RenderBox || !content.hasSize) {
      return Rect.zero;
    }
    // Resolve after layout, so toolbar heights, client geometry changes and
    // interactive resizes cannot leave a stale exclusion over visible chrome.
    RenderObject? ancestor = content;
    while (ancestor != null && ancestor != this) {
      ancestor = ancestor.parent;
    }
    if (ancestor == null) return Rect.zero;
    final translation =
        MatrixUtils.getAsTranslation(content.getTransformTo(this));
    if (translation == null) return Rect.zero; // Unknown transforms: keep blur.
    return _opaqueRect.intersect(Offset.zero & content.size).shift(translation);
  }

  Rect get opaquePaintRect {
    final covered = _contentOpaqueRect()
        .intersect(
            _radius.toRRect(Offset.zero & size).scaleRadii().safeInnerRect)
        .deflate(1);
    return covered.isEmpty || !covered.isFinite ? Rect.zero : covered;
  }

  List<Rect> get backdropPaintRects {
    if (_sigma <= 0) return const [];
    final bounds = Offset.zero & size;
    return exposedBackdropRects(
        bounds, _coveredRect(_radius.toRRect(bounds).scaleRadii()));
  }

  double get backdropOutset => 3 * _sigma + 4;

  Rect _coveredRect(RRect clip) {
    final covered = _contentOpaqueRect();
    // Skia does not implement shared backdrop IDs. Adjacent strips would
    // sample each other's blur. Only split across a full-width/full-height
    // opaque band wider than both filters' sampling support (3 sigma each).
    // Keep rounded/antialiased corners outside the exclusion too.
    final separation = 6 * _sigma + 4;
    if (covered.left <= 0 && covered.right >= size.width) {
      final band = covered.intersect(Rect.fromLTRB(
          0,
          clip.safeInnerRect.top + 1,
          size.width,
          clip.safeInnerRect.bottom - 1));
      if (band.height > separation) return band;
    }
    if (covered.top <= 0 && covered.bottom >= size.height) {
      final band = covered.intersect(Rect.fromLTRB(clip.safeInnerRect.left + 1,
          0, clip.safeInnerRect.right - 1, size.height));
      if (band.width > separation) return band;
    }
    return Rect.zero;
  }

  @override
  void paint(PaintingContext context, Offset offset) {
    if (_sigma > 0 && !size.isEmpty) {
      final bounds = Offset.zero & size;
      final clip = _radius.toRRect(bounds).scaleRadii();
      final regions = exposedBackdropRects(bounds, _coveredRect(clip));
      FlutterFrameTrace.backdrop(bounds.width * bounds.height,
          regions.fold(0.0, (area, rect) => area + rect.width * rect.height));
      context.pushClipRRect(true, offset, bounds, clip, (context, offset) {
        for (final region in regions) {
          context.pushClipRect(true, offset, region, (context, offset) {
            context.pushLayer(
              BackdropFilterLayer(
                  filter: ui.ImageFilter.blur(sigmaX: _sigma, sigmaY: _sigma)),
              (context, offset) {},
              offset,
            );
          });
        }
      });
    }
    super.paint(context, offset);
  }
}
