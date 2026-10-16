import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/util/backend_surface_geometry.dart';
import 'package:jdwm/src/util/rect_overflow_box.dart';

void main() {
  const geometry = Rect.fromLTWH(50, 50, 700, 500);
  const surface = Size(800, 600);

  testWidgets('client shadow margins are removed without clipping content',
      (tester) async {
    final crop = backendManagedSurfaceRect(
      surfaceSize: surface,
      visibleBounds: geometry,
      bufferSourceBox: Offset.zero & surface,
      bufferSize: surface,
    );
    const viewportKey = ValueKey('viewport');
    const contentKey = ValueKey('content');
    await tester.pumpWidget(Directionality(
      textDirection: TextDirection.ltr,
      child: Align(
        alignment: Alignment.topLeft,
        child: ClipRect(
          key: viewportKey,
          child: RectOverflowBox(
            rect: crop,
            child: SizedBox.fromSize(
              size: surface,
              child: Stack(children: [
                Positioned.fromRect(
                  rect: geometry,
                  child: const ColoredBox(key: contentKey, color: Colors.red),
                ),
              ]),
            ),
          ),
        ),
      ),
    ));
    expect(tester.getRect(find.byKey(contentKey)),
        tester.getRect(find.byKey(viewportKey)));
    expect(tester.getSize(find.byKey(viewportKey)), geometry.size);
  });

  test('window geometry stays authoritative for scaled and cropped buffers',
      () {
    for (final source in [
      const Rect.fromLTWH(0, 0, 1600, 1200),
      const Rect.fromLTWH(100, 80, 1600, 1200),
    ]) {
      expect(
        backendManagedSurfaceRect(
          surfaceSize: surface,
          visibleBounds: geometry,
          bufferSourceBox: source,
          bufferSize: const Size(1800, 1400),
        ),
        geometry,
      );
      expect(
        backendUsesSurfaceDestinationSize(
          surfaceSize: surface,
          visibleBounds: geometry,
          bufferSourceBox: source,
          bufferSize: const Size(1800, 1400),
        ),
        isFalse,
      );
    }
  });

  test('surfaces without window geometry use their destination size', () {
    expect(
      backendManagedSurfaceRect(
        surfaceSize: surface,
        visibleBounds: Rect.zero,
        bufferSourceBox: const Rect.fromLTWH(100, 80, 1600, 1200),
        bufferSize: const Size(1800, 1400),
      ),
      Offset.zero & surface,
    );
  });
}
