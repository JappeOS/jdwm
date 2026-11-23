import 'package:flutter/gestures.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/core/models/surface_role.dart';
import 'package:jdwm/src/ui/common/view_input_listener.dart';

void main() {
  testWidgets('moving a client window keeps the released pointer in its header',
      (tester) async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final calls = <MethodCall>[];
    const channel = MethodChannel('platform');
    final messenger = tester.binding.defaultBinaryMessenger;
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return null;
    });
    addTearDown(() => messenger.setMockMethodCallHandler(channel, null));
    const rect = Rect.fromLTWH(0, 0, 200, 200);
    container.read(surfaceStatesProvider(1).notifier).commit(
      role: SurfaceRole.xdgSurface,
      textureId: 1,
      surfacePosition: Offset.zero,
      surfaceSize: rect.size,
      scale: 1,
      bufferSourceBox: rect,
      bufferSize: rect.size,
      inputRegion: const Rect.fromLTWH(4, 4, 192, 192),
      subsurfacesBelow: [],
      subsurfacesAbove: [],
    );
    final top = ValueNotifier(100.0);
    addTearDown(top.dispose);
    await tester.pumpWidget(UncontrolledProviderScope(
      container: container,
      child: Directionality(
        textDirection: TextDirection.ltr,
        child: ValueListenableBuilder<double>(
          valueListenable: top,
          builder: (context, y, child) => Stack(children: [
            Positioned(
                left: 20, top: y, width: 200, height: 200, child: child!),
          ]),
          child: Listener(
            onPointerMove: (event) => top.value += event.delta.dy,
            child: const ViewInputListener(viewId: 1, child: SizedBox()),
          ),
        ),
      ),
    ));
    final mouse = await tester.createGesture(kind: PointerDeviceKind.mouse);
    Offset inHeader() => Offset(100, top.value + 18);
    await mouse.addPointer(location: inHeader());
    for (final dy in [-30.0, 30.0]) {
      for (final layoutBeforeRelease in [true, false]) {
        calls.clear();
        final start = inHeader();
        await mouse.down(start);
        for (final fraction in [1 / 3, 2 / 3, 1.0]) {
          await mouse.moveTo(start.translate(0, dy * fraction));
          if (fraction < 1 || layoutBeforeRelease) await tester.pump();
        }
        await mouse.up();
        await tester.pump();
        expect(top.value + 18, start.dy + dy);
        final lastPointer = calls.lastWhere(
            (c) => c.method == 'pointer_hover' || c.method == 'pointer_exit');
        expect(lastPointer.method, 'pointer_hover');
        expect(lastPointer.arguments, {'view_id': 1, 'x': 80.0, 'y': 18.0});
      }
    }
    await mouse.removePointer();
  });

  testWidgets('removing the hovered surface hands off a stationary pointer',
      (tester) async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final calls = <MethodCall>[];
    final messenger = tester.binding.defaultBinaryMessenger;
    const channel = MethodChannel('platform');
    messenger.setMockMethodCallHandler(channel, (call) async {
      calls.add(call);
      return null;
    });
    addTearDown(() => messenger.setMockMethodCallHandler(channel, null));
    const rect = Rect.fromLTWH(0, 0, 100, 100);
    for (final id in [1, 2]) {
      container.read(surfaceStatesProvider(id).notifier).commit(
        role: SurfaceRole.xdgSurface,
        textureId: id,
        surfacePosition: Offset.zero,
        surfaceSize: rect.size,
        scale: 1,
        bufferSourceBox: rect,
        bufferSize: rect.size,
        inputRegion: rect,
        subsurfacesBelow: [],
        subsurfacesAbove: [],
      );
    }
    Future<void> show(List<int> ids) async {
      await tester.pumpWidget(UncontrolledProviderScope(
        container: container,
        child: Directionality(
          textDirection: TextDirection.ltr,
          child: Stack(children: [
            for (final id in ids)
              Positioned(
                key: ValueKey(id),
                left: id * 10.0,
                top: 10,
                width: 100,
                height: 100,
                child: ViewInputListener(viewId: id, child: const SizedBox()),
              ),
          ]),
        ),
      ));
      await tester.pump();
    }

    final mouse = await tester.createGesture(kind: PointerDeviceKind.mouse);
    await show([1, 2]);
    await mouse.addPointer(location: const Offset(50, 50));
    await mouse.moveTo(const Offset(55, 50));
    await tester.pump();
    expect(calls.last.arguments['view_id'], 2);
    calls.clear();
    await show([2]); // Removing a different window preserves ownership.
    expect(calls, isEmpty);
    await show([]);
    expect(calls.map((c) => c.method), ['pointer_exit']);

    await show([1, 2]);
    calls.clear();
    await mouse.down(const Offset(55, 50));
    await tester.pump();
    await show([1]); // The pressed surface disappears over another client.
    expect(
        calls
            .where((c) => c.method == 'mouse_button_event')
            .map((c) => c.arguments['is_pressed']),
        [true, false]);
    expect(calls.last.method, 'pointer_hover');
    expect(calls.last.arguments, {'view_id': 1, 'x': 45.0, 'y': 40.0});
    expect(calls.where((c) => c.method == 'pointer_exit'), isEmpty);
    await mouse.up();
    calls.clear();
    await mouse.moveTo(const Offset(200, 200));
    await tester.pump();
    expect(calls.map((c) => c.method), ['pointer_exit']);
    // Keep routing a drag to the pressed surface until the button comes up.
    await show([1, 2]);
    await mouse.moveTo(const Offset(55, 50));
    await mouse.down(const Offset(55, 50));
    calls.clear();
    await mouse.moveTo(const Offset(15, 25));
    await mouse.moveTo(const Offset(12, 75));
    await tester.pump();
    expect(
        calls
            .where((c) => c.method == 'pointer_hover')
            .map((c) => c.arguments['view_id']),
        everyElement(2));
    await mouse.up();
    await tester.pump();
    expect(calls.last.arguments, {'view_id': 1, 'x': 2.0, 'y': 65.0});
    expect(calls.where((c) => c.method == 'pointer_exit'), isEmpty);
    await mouse.removePointer();
  });
}
