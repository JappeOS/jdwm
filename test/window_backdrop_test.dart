import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/ui/common/window_backdrop.dart';

void main() {
  testWidgets('exposed blur pixels match the full backdrop filter',
      (tester) async {
    final captureKey = GlobalKey();
    final contentKey = GlobalKey();
    Future<List<int>> capture(Rect opaque, double sigma) async {
      await tester.pumpWidget(Directionality(
        textDirection: TextDirection.ltr,
        child: Align(
            alignment: Alignment.topLeft,
            child: RepaintBoundary(
              key: captureKey,
              child: SizedBox(
                  width: 200,
                  height: 400,
                  child: Stack(children: [
                    for (var i = 0; i < 10; i++)
                      Positioned(
                          left: i * 20,
                          top: 0,
                          bottom: 0,
                          width: 20,
                          child: ColoredBox(
                              color: i.isEven
                                  ? const Color(0xffaa2244)
                                  : const Color(0xff22aa44))),
                    Positioned.fill(
                        child: WindowBackdrop(
                      sigma: sigma,
                      radius: BorderRadius.circular(8),
                      contentKey: contentKey,
                      opaqueRect: opaque,
                      child: RepaintBoundary(
                          key: contentKey,
                          child: Stack(children: const [
                            Positioned(
                                left: 0,
                                top: 60,
                                width: 200,
                                height: 280,
                                child: ColoredBox(color: Color(0xff222222))),
                          ])),
                    )),
                  ])),
            )),
      ));
      expect(tester.layers.whereType<BackdropFilterLayer>().length,
          opaque.isEmpty ? 1 : 2);
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

    for (final sigma in [8.0, 30.0]) {
      final full = await capture(Rect.zero, sigma);
      final clipped =
          await capture(const Rect.fromLTWH(0, 60, 200, 280), sigma);
      expect(clipped.length, full.length);
      var maximumDifference = 0;
      for (var i = 0; i < full.length; i++) {
        final difference = (full[i] - clipped[i]).abs();
        if (difference > maximumDifference) maximumDifference = difference;
      }
      expect(maximumDifference, lessThanOrEqualTo(2));
    }
  });

  test('blur strips cover only the exposed area without overlaps', () {
    const bounds = Rect.fromLTWH(0, 0, 1000, 800);
    const opaque = Rect.fromLTWH(0, 40, 1000, 752);
    final strips = exposedBackdropRects(bounds, opaque);
    expect(strips.length, 2);
    var area = 0.0;
    for (var i = 0; i < strips.length; i++) {
      expect(strips[i].overlaps(opaque), isFalse);
      for (var j = i + 1; j < strips.length; j++) {
        expect(strips[i].overlaps(strips[j]), isFalse);
      }
      area += strips[i].width * strips[i].height;
    }
    expect(area, bounds.width * bounds.height - opaque.width * opaque.height);
    expect(area / (bounds.width * bounds.height), lessThan(0.08));
    expect(exposedBackdropRects(bounds, Rect.zero), [bounds]);
    expect(exposedBackdropRects(bounds, bounds), isEmpty);
    expect(exposedBackdropRects(bounds, bounds.inflate(10)), isEmpty);
  });

  testWidgets(
      'uses actual content layout and restores blur when opacity changes',
      (tester) async {
    final contentKey = GlobalKey();
    Widget window({required double toolbarHeight, required Rect opaque}) {
      return Directionality(
        textDirection: TextDirection.ltr,
        child: Align(
          alignment: Alignment.topLeft,
          child: SizedBox(
            width: 400,
            height: 300,
            child: WindowBackdrop(
              sigma: 30,
              radius: BorderRadius.circular(8),
              contentKey: contentKey,
              opaqueRect: opaque,
              child: Column(children: [
                SizedBox(height: toolbarHeight),
                Expanded(
                    child: RepaintBoundary(
                  key: contentKey,
                  child: const ColoredBox(
                      color: Color(0xff111111), child: SizedBox.expand()),
                )),
              ]),
            ),
          ),
        ),
      );
    }

    List<Rect> filterRects() => tester.layers
        .whereType<BackdropFilterLayer>()
        .map((layer) => (layer.parent! as ClipRectLayer).clipRect!)
        .toList();
    const body = Rect.fromLTWH(0, 0, 400, 300);
    await tester.pumpWidget(window(toolbarHeight: 30, opaque: body));
    final originalContent = contentKey.currentContext!.findRenderObject();
    expect(filterRects().any((rect) => rect.contains(const Offset(200, 150))),
        isFalse);
    expect(filterRects().any((rect) => rect.contains(const Offset(200, 20))),
        isTrue);
    await tester.pumpWidget(window(toolbarHeight: 90, opaque: body));
    expect(filterRects().any((rect) => rect.contains(const Offset(200, 80))),
        isTrue);
    expect(filterRects().any((rect) => rect.contains(const Offset(200, 150))),
        isFalse);

    await tester.pumpWidget(window(toolbarHeight: 90, opaque: Rect.zero));
    expect(filterRects(), [const Rect.fromLTWH(0, 0, 400, 300)]);
    expect(
        contentKey.currentContext!.findRenderObject(), same(originalContent));
    // Small bands and irregular transparency retain one original filter;
    // splitting them would allow one blur to sample another blur's output.
    await tester.pumpWidget(
        window(toolbarHeight: 90, opaque: const Rect.fromLTWH(0, 0, 400, 40)));
    expect(filterRects(), [const Rect.fromLTWH(0, 0, 400, 300)]);
    await tester.pumpWidget(window(
        toolbarHeight: 90, opaque: const Rect.fromLTWH(30, 0, 340, 210)));
    expect(filterRects(), [const Rect.fromLTWH(0, 0, 400, 300)]);
  });
}
