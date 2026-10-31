import 'dart:async';
import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/jdwm.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;

void main() {
  testWidgets('an empty backend inventory removes monitors and can recover',
      (tester) async {
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
        const MethodChannel('platform'), (_) async => null);
    final key = GlobalKey<WindowManagerState>();
    await tester.pumpWidget(shadcn.Theme(
      data: const shadcn.ThemeData(),
      child: Directionality(
        textDirection: TextDirection.ltr,
        child: WindowManager(
          key: key,
          monitorBuilder: (_, monitor) =>
              SizedBox(key: ValueKey('monitor:${monitor.id}')),
        ),
      ),
    ));
    expect(find.byKey(const ValueKey('monitor:__jdwm_auto_monitor__')),
        findsOneWidget);
    Future<void> monitors(List<Map<String, Object>> value) async {
      final done = Completer<void>();
      tester.binding.channelBuffers.push(
          'platform',
          const StandardMethodCodec()
              .encodeMethodCall(MethodCall('set_monitors', value)),
          (_) => done.complete());
      await done.future;
      await tester.pump();
      await tester.pump();
    }

    // Empty is authoritative even when it is the first backend message.
    await monitors([]);
    expect(find.byKey(const ValueKey('monitor:__jdwm_auto_monitor__')),
        findsNothing);
    const left = {
      'id': 'left',
      'x': 0,
      'y': 0,
      'width': 960,
      'height': 540,
      'is_primary': true
    };
    const right = {
      'id': 'right',
      'x': 960,
      'y': 0,
      'width': 1080,
      'height': 1920,
      'is_primary': false
    };
    await monitors([left, right]);
    expect(find.byKey(const ValueKey('monitor:left')), findsOneWidget);
    expect(key.currentState!.getMonitorById('right')!.bounds,
        const Rect.fromLTWH(960, 0, 1080, 1920));
    final window = WindowEntry(
        icon: const AssetImage('unused'), content: const SizedBox.expand());
    key.currentState!.pushWindow(window, monitorId: 'right');
    window.windowRect = const Rect.fromLTWH(1000, 100, 200, 180);
    await tester.pump();
    window.fullscreen = true;
    await tester.pump();
    await monitors([left]);
    expect(find.byKey(const ValueKey('monitor:right')), findsNothing);
    expect(window.monitorId, 'left');
    expect(key.currentState!.targetRectForWindow(window),
        const Rect.fromLTWH(0, 0, 960, 540));
    window.fullscreen = false;
    await tester.pump();
    expect(window.windowRect.left, 40);
    expect(window.windowRect.top, 100);
    await monitors([]);
    expect(find.byKey(const ValueKey('monitor:left')), findsNothing);
    expect(find.byKey(const ValueKey('monitor:__jdwm_auto_monitor__')),
        findsNothing);
    await monitors([left]);
    expect(find.byKey(const ValueKey('monitor:left')), findsOneWidget);
    await tester.pumpWidget(const SizedBox());
    window.dispose();
    tester.binding.defaultBinaryMessenger
        .setMockMethodCallHandler(const MethodChannel('platform'), null);
  });
}
