import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/ui/common/window_backdrop.dart';
import 'package:jdwm/src/util/flutter_frame_trace.dart';

void main() {
  testWidgets('diagnostic blur control preserves content and theme updates',
      (tester) async {
    final contentKey = GlobalKey();
    Widget scene(double sigma) => Directionality(
          textDirection: TextDirection.ltr,
          child: Center(
              child: SizedBox(
            width: 300,
            height: 200,
            child: WindowBackdrop(
              sigma: sigma,
              radius: BorderRadius.circular(8),
              contentKey: contentKey,
              opaqueRect: Rect.zero,
              child: RepaintBoundary(
                  key: contentKey,
                  child: const ColoredBox(color: Color(0xff112233))),
            ),
          )),
        );
    await tester.pumpWidget(scene(30));
    final content = contentKey.currentContext!.findRenderObject();
    expect(tester.layers.whereType<BackdropFilterLayer>().length,
        windowBlurDisabled ? 0 : 1);
    await tester.pumpWidget(scene(0));
    expect(tester.layers.whereType<BackdropFilterLayer>(), isEmpty);
    await tester.pumpWidget(scene(20));
    expect(tester.layers.whereType<BackdropFilterLayer>().length,
        windowBlurDisabled ? 0 : 1);
    expect(contentKey.currentContext!.findRenderObject(), same(content));
  });
}
