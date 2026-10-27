import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/ui/common/window_backdrop.dart';
import 'package:jdwm/src/ui/common/window_occlusion_stack.dart';
import 'package:jdwm/src/util/window_occlusion.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;

void main() {
  testWidgets('30 windows skip covered paints and reveal retained content',
      (tester) async {
    final keys = List.generate(30, (_) => GlobalKey());
    final painters = List.generate(30, (_) => _PaintCounter());
    Widget window(int i, Rect rect) => Positioned.fill(
            child: WindowPaintBoundary(
          child: Stack(children: [
            Positioned.fromRect(
                rect: rect,
                child: WindowBackdrop(
                  sigma: 0,
                  radius: BorderRadius.zero,
                  contentKey: keys[i],
                  opaqueRect: Offset.zero & rect.size,
                  child: RepaintBoundary(
                      key: keys[i], child: CustomPaint(painter: painters[i])),
                ))
          ]),
        ));
    final stationary = [
      for (var i = 0; i < 29; i++)
        window(i, const Rect.fromLTWH(200, 200, 120, 100))
    ];
    Future<void> show(Rect top) => tester.pumpWidget(Directionality(
          textDirection: TextDirection.ltr,
          child: Center(
              child: SizedBox(
            width: 600,
            height: 600,
            child: WindowOcclusionStack(
                contentKeys: keys, children: [...stationary, window(29, top)]),
          )),
        ));

    await show(const Rect.fromLTWH(0, 0, 600, 600));
    expect(painters.take(29).every((p) => p.paints == 0), isTrue);
    expect(painters.last.paints, 1);
    // Every hidden widget stayed mounted; moving the covering window reveals
    // the correct content in the same frame, without a client recommit.
    expect(keys.every((key) => key.currentContext != null), isTrue);
    await show(const Rect.fromLTWH(500, 0, 600, 600));
    expect(painters.take(29).every((p) => p.paints > 0), isTrue);
    final recorded = painters.map((p) => p.paints).toList();
    await show(const Rect.fromLTWH(510, 0, 600, 600));
    expect(painters.map((p) => p.paints).toList(), recorded);
  });

  test('opaque coverage retains the input of visible backdrop filters', () {
    final region = WindowPaintRegion(const Rect.fromLTWH(0, 0, 400, 300));
    final visible = List<Rect>.of(region.rects);
    region.subtract(const Rect.fromLTWH(10, 10, 380, 280));
    expect(region.rects.any((r) => r.contains(const Offset(200, 30))), isFalse);
    region.requireBackdrop(visible, [const Rect.fromLTWH(0, 0, 400, 10)], 40);
    expect(region.rects.any((r) => r.contains(const Offset(200, 30))), isTrue);
    expect(
        region.rects.any((r) => r.contains(const Offset(200, 150))), isFalse);
    for (var i = 0; i < region.rects.length; i++) {
      for (var j = i + 1; j < region.rects.length; j++) {
        expect(region.rects[i].overlaps(region.rects[j]), isFalse);
      }
    }
    // Highly fragmented scenes conservatively overdraw rather than grow
    // unbounded geometry or lose required pixels.
    for (var i = 0; i < 200; i++) {
      region.subtract(Rect.fromLTWH(i * 2, 0, 1, 300));
    }
    expect(region.rects.length, lessThanOrEqualTo(WindowPaintRegion.maxRects));
  });

  testWidgets(
      'occluded stack matches full painting including blur, shadows and moves',
      (tester) async {
    final captureKey = GlobalKey();
    final contentKeys = List.generate(5, (_) => GlobalKey());
    Widget window(
        int index, Offset shift, bool transparent, double sigma, bool chrome) {
      final key = contentKeys[index];
      const width = 230.0, height = 280.0;
      Widget content = RepaintBoundary(
          key: key,
          child: DecoratedBox(
            decoration: const BoxDecoration(boxShadow: [
              BoxShadow(
                  color: Color(0x66000000),
                  blurRadius: 10,
                  offset: Offset(0, 5)),
            ]),
            child: ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: Stack(children: [
                  const Positioned.fill(
                      child: ColoredBox(color: Color(0x66888888))),
                  Positioned(
                      left: 0,
                      right: 0,
                      top: 12,
                      bottom: 12,
                      child: ColoredBox(
                          color: Color(0xff203040 + index * 0x15100c))),
                ])),
          ));
      if (chrome) {
        content = shadcn.DualBorderOutlinedContainer(
          borderRadius: BorderRadius.circular(9),
          surfaceBlur: 0,
          surfaceOpacity: 0.5,
          boxShadow: const [
            BoxShadow(
                color: Color(0x4b000000),
                blurRadius: 20,
                spreadRadius: 3,
                offset: Offset(0, 10)),
          ],
          child: Column(children: [Expanded(child: content)]),
        );
      }
      Widget backdrop = WindowBackdrop(
        sigma: sigma,
        radius: BorderRadius.circular(8),
        contentKey: key,
        opaqueRect: const Rect.fromLTWH(0, 12, width, height - 24),
        child: content,
      );
      if (transparent) backdrop = Opacity(opacity: 0.5, child: backdrop);
      return Positioned.fill(
          child: WindowPaintBoundary(
              child: Stack(clipBehavior: Clip.none, children: [
        Positioned(
            left: 15 + shift.dx,
            top: 15 + shift.dy,
            width: width,
            height: height,
            child: backdrop),
      ])));
    }

    Future<List<int>> capture(bool occlusion, Offset top,
        {bool transparent = false,
        double sigma = 6,
        bool chrome = false}) async {
      final windows = [
        for (var i = 0; i < contentKeys.length; i++)
          window(i, i == contentKeys.length - 1 ? top : Offset(i * 8, i * 5),
              transparent && i == contentKeys.length - 1, sigma, chrome)
      ];
      await tester.pumpWidget(Directionality(
        textDirection: TextDirection.ltr,
        child: shadcn.Theme(
            data: const shadcn.ThemeData(),
            child: Align(
                alignment: Alignment.topLeft,
                child: RepaintBoundary(
                  key: captureKey,
                  child: SizedBox(
                      width: 320,
                      height: 340,
                      child: Stack(children: [
                        for (var i = 0; i < 16; i++)
                          Positioned(
                              left: i * 20,
                              top: 0,
                              bottom: 0,
                              width: 20,
                              child: ColoredBox(
                                  color: i.isEven
                                      ? const Color(0xffdd7722)
                                      : const Color(0xff3388bb))),
                        Positioned.fill(
                            child: occlusion
                                ? WindowOcclusionStack(
                                    contentKeys: contentKeys, children: windows)
                                : Stack(
                                    clipBehavior: Clip.none,
                                    children: windows)),
                      ])),
                ))),
      ));
      if (occlusion) {
        expect(tester.layers.whereType<ClipPathLayer>(), isNotEmpty);
        // The chrome itself also uses ClipPathLayer. Require a clip on a direct
        // window boundary to prove the production wrapper path was recognized.
        final boundaries = tester.renderObjectList<RenderRepaintBoundary>(
            find.byType(WindowPaintBoundary));
        expect(boundaries.any((b) => b.debugLayer?.parent is ClipPathLayer),
            isTrue);
      }
      return (await tester.runAsync(() async {
        final boundary = captureKey.currentContext!.findRenderObject()!
            as RenderRepaintBoundary;
        final image = await boundary.toImage();
        final bytes =
            await image.toByteData(format: ui.ImageByteFormat.rawRgba);
        image.dispose();
        return bytes!.buffer.asUint8List().toList();
      }))!;
    }

    Future<void> compare(Offset offset,
        {bool transparent = false,
        double sigma = 6,
        bool chrome = false}) async {
      final full = await capture(false, offset,
          transparent: transparent, sigma: sigma, chrome: chrome);
      final clipped = await capture(true, offset,
          transparent: transparent, sigma: sigma, chrome: chrome);
      var maxDifference = 0;
      for (var i = 0; i < full.length; i++) {
        final difference = (full[i] - clipped[i]).abs();
        if (difference > maxDifference) maxDifference = difference;
      }
      expect(maxDifference, lessThanOrEqualTo(2));
    }

    await compare(const Offset(24, 18));
    await capture(
        true, const Offset(70, 25)); // Move retained windows before comparing.
    await compare(const Offset(70, 25));
    await compare(
        const Offset(-55, -20)); // Partially offscreen, including its shadow.
    await compare(const Offset(24, 18), transparent: true);
    await compare(const Offset(24, 18), sigma: 30, chrome: true);
  });
}

class _PaintCounter extends CustomPainter {
  int paints = 0;
  @override
  void paint(Canvas canvas, Size size) {
    paints++;
    canvas.drawRect(
        Offset.zero & size, Paint()..color = const Color(0xff225588));
  }

  @override
  bool shouldRepaint(_PaintCounter oldDelegate) => false;
}
