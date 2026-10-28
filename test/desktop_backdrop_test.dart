import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;

void main() {
  testWidgets(
      'full scene keeps dock padding and top bar blurred during hover and tooltip changes',
      (tester) async {
    final captureKey = GlobalKey();
    final buttonKey = GlobalKey();
    final hover = ValueNotifier(false);
    final blur = ValueNotifier(true);
    await tester.pumpWidget(Align(
      alignment: Alignment.topLeft,
      child: RepaintBoundary(
        key: captureKey,
        child: SizedBox(
          width: 320,
          height: 360,
          child: shadcn.ShadcnApp(
            theme: const shadcn.ThemeData(surfaceBlur: 6, surfaceOpacity: 0.35),
            home: Stack(children: [
              for (var x = 0; x < 320; x += 8)
                Positioned(
                    left: x.toDouble(),
                    top: 0,
                    bottom: 0,
                    width: 8,
                    child: ColoredBox(
                        color: x % 16 == 0
                            ? const Color(0xff225599)
                            : const Color(0xffeebb44))),
              Positioned(
                  left: 0,
                  top: 0,
                  right: 0,
                  height: 28,
                  child: shadcn.SurfaceBlur(
                      surfaceBlur: 6,
                      child: const ColoredBox(color: Color(0x44555555)))),
              Positioned(
                  left: 124,
                  top: 280,
                  width: 72,
                  height: 64,
                  child: ValueListenableBuilder<bool>(
                    valueListenable: blur,
                    builder: (context, enabled, child) =>
                        shadcn.DualBorderOutlinedContainer(
                      borderRadius: BorderRadius.circular(12),
                      padding: const EdgeInsets.all(5),
                      surfaceOpacity: 0.35,
                      surfaceBlur: enabled ? 6 : 0,
                      child: child!,
                    ),
                    child: shadcn.Tooltip(
                      alignment: const Alignment(0, -1.15),
                      tooltip: (_) => const shadcn.TooltipContainer(
                          child: Text('Launcher')),
                      child: MouseRegion(
                        key: buttonKey,
                        onEnter: (_) => hover.value = true,
                        onExit: (_) => hover.value = false,
                        child: ValueListenableBuilder<bool>(
                          valueListenable: hover,
                          builder: (context, hovered, _) => ColoredBox(
                            color: hovered
                                ? const Color(0x44999999)
                                : const Color(0x00000000),
                          ),
                        ),
                      ),
                    ),
                  )),
            ]),
          ),
        ),
      ),
    ));
    await tester.pumpAndSettle();

    Future<List<int>> capture() async => (await tester.runAsync(() async {
          final boundary = captureKey.currentContext!.findRenderObject()!
              as RenderRepaintBoundary;
          final image = await boundary.toImage();
          final bytes =
              await image.toByteData(format: ui.ImageByteFormat.rawRgba);
          image.dispose();
          return bytes!.buffer.asUint8List().toList();
        }))!;

    List<int> pixels(List<int> image, Rect region) => [
          for (var y = region.top.toInt(); y < region.bottom; y++)
            for (var x = region.left.toInt(); x < region.right; x++)
              ...image.sublist((y * 320 + x) * 4, (y * 320 + x) * 4 + 4),
        ];

    const padding = Rect.fromLTWH(126, 302, 2, 18);
    const topBar = Rect.fromLTWH(20, 5, 280, 16);
    final baseline = await capture();
    blur.value = false;
    await tester.pumpAndSettle();
    expect(pixels(await capture(), padding), isNot(pixels(baseline, padding)),
        reason:
            'The padding comparison must actually distinguish blurred pixels.');
    blur.value = true;
    await tester.pumpAndSettle();

    Future<void> checkBackdrop() async {
      final current = await capture();
      expect(pixels(current, padding), pixels(baseline, padding));
      expect(pixels(current, topBar), pixels(baseline, topBar));
    }

    final mouse = await tester.createGesture(kind: ui.PointerDeviceKind.mouse);
    await mouse.addPointer(location: const Offset(310, 200));
    for (var cycle = 0; cycle < 2; cycle++) {
      await mouse.moveTo(tester.getCenter(find.byKey(buttonKey)));
      await tester.pump();
      expect(hover.value, isTrue);
      await checkBackdrop();
      await tester.pump(const Duration(milliseconds: 600));
      await tester.pump(const Duration(milliseconds: 100));
      expect(find.text('Launcher'), findsOneWidget);
      await checkBackdrop();
      await tester.pump(const Duration(milliseconds: 300));
      await checkBackdrop();
      await mouse.moveTo(const Offset(310, 200));
      await tester.pump();
      expect(hover.value, isFalse);
      await checkBackdrop();
      await tester.pumpAndSettle();
      expect(find.text('Launcher'), findsNothing);
      await checkBackdrop();
    }
    await mouse.removePointer();
    await tester.pumpWidget(const SizedBox());
    hover.dispose();
    blur.dispose();
  });
}
