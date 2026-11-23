import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart' as riverpod;
import 'package:flutter_test/flutter_test.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;
import 'package:jdwm/src/window_manager.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/ui/common/xdg_toplevel_surface.dart';

Future<GlobalKey<WindowManagerState>> mountManager(
    WidgetTester tester, List<MethodCall> calls) async {
  tester.binding.defaultBinaryMessenger
      .setMockMethodCallHandler(const MethodChannel('platform'), (call) async {
    calls.add(call);
    return null;
  });
  final key = GlobalKey<WindowManagerState>();
  const bounds = Rect.fromLTWH(100, 0, 600, 500);
  await tester.pumpWidget(shadcn.Theme(
    data: const shadcn.ThemeData(),
    child: Directionality(
      textDirection: TextDirection.ltr,
      child: WindowManager(key: key, monitors: const [
        MonitorConfig(id: 'main', bounds: bounds, margin: EdgeInsets.all(30)),
      ]),
    ),
  ));
  return key;
}

Future<void> sendEvent(
    WidgetTester tester, String method, int viewId, String protocol,
    [Map<String, Object> data = const {}]) async {
  final done = Completer<void>();
  tester.binding.channelBuffers.push(
      'platform',
      const StandardMethodCodec().encodeMethodCall(MethodCall(method, {
        'view_id': viewId,
        'protocol': protocol,
        ...data,
      })),
      (_) => done.complete());
  await done.future;
  await tester.pump();
  await tester.pump();
}

void main() {
  testWidgets('one click transfers focus to an older or newer client',
      (tester) async {
    final calls = <MethodCall>[];
    final key = await mountManager(tester, calls);
    final manager = key.currentState!;
    for (final (id, protocol) in [(1, 'xdg'), (2, 'xwayland')]) {
      await sendEvent(tester, 'commit_toplevel_surface', id, protocol,
          {'has_decoration': true, 'decoration': 2});
      await sendEvent(tester, 'map_toplevel_surface', id, protocol);
    }

    final windows = manager.getAllWindows();
    windows[0].windowRect = const Rect.fromLTWH(40, 40, 360, 260);
    windows[1].windowRect = const Rect.fromLTWH(280, 40, 360, 260);
    await tester.pump();
    final container = riverpod.ProviderScope.containerOf(
        tester.element(find.byType(XdgToplevelSurface).first),
        listen: false);
    for (final (id, x) in [(1, 160.0), (2, 440.0)]) {
      calls.clear();
      // Click the exposed title bar of the currently lower window.
      await tester.tapAt(Offset(x, 56));
      await tester.pump();
      expect(manager.getManagedWindowsByFocus().last.viewId, id);
      expect(container.read(xdgToplevelStatesProvider(id)).focusNode.hasFocus,
          isTrue);
      final previousId = id == 1 ? 2 : 1;
      expect(
          calls
              .where((c) => c.method == 'activate_window')
              .map((c) => c.arguments),
          unorderedEquals([
            [previousId, false],
            [id, true]
          ]));
    }
    await tester.pumpWidget(const SizedBox());
    tester.binding.defaultBinaryMessenger
        .setMockMethodCallHandler(const MethodChannel('platform'), null);
  });

  for (final protocol in ['xdg', 'xwayland']) {
    testWidgets('$protocol activation restores a minimized topmost window',
        (tester) async {
      final calls = <MethodCall>[];
      final key = await mountManager(tester, calls);
      Future<void> event(String method,
              [Map<String, Object> data = const {}]) =>
          sendEvent(tester, method, 1, protocol, data);

      await event(
          'commit_toplevel_surface', {'has_decoration': true, 'decoration': 2});
      await event('map_toplevel_surface');
      final manager = key.currentState!;
      final window = manager.getAllWindows().single;
      final container = riverpod.ProviderScope.containerOf(
          tester.element(find.byType(XdgToplevelSurface)),
          listen: false);
      final original = window.windowRect;
      calls.clear();
      container.read(xdgToplevelStatesProvider(1).notifier).requestMinimize();
      await tester.pump();
      expect(
          calls.where((call) => call.method == 'change_window_visibility')
              .map((call) => call.arguments),
          [
            {'view_id': 1, 'visible': false}
          ]);
      expect(window.minimized, isFalse);
      await event('set_toplevel_state', {'visible': false, 'maximized': false});
      expect(window.minimized, isTrue);
      expect(manager.getManagedWindowsByFocus().single.visible, isFalse);
      calls.clear();

      manager.activateWindow(window);
      await tester.pump();
      expect(
          calls
              .where((call) => call.method == 'change_window_visibility')
              .map((call) => call.arguments),
          [
            {'view_id': 1, 'visible': true}
          ]);
      // Visibility is confirmed by the backend, not optimistically overwritten.
      expect(window.minimized, isTrue);
      await event('set_toplevel_state', {'visible': true, 'maximized': false});
      await tester.pump();
      expect(window.minimized, isFalse);
      expect(window.windowRect, original);
      expect(container.read(xdgToplevelStatesProvider(1)).focusNode.hasFocus,
          isTrue);
      expect(
          calls.any((call) =>
              call.method == 'activate_window' && call.arguments[1] == true),
          isTrue);
      calls.clear();
      manager.activateWindow(window);
      await tester.pump();
      expect(calls.where((call) => call.method == 'change_window_visibility'),
          isEmpty);

      await tester.pumpWidget(const SizedBox());
      tester.binding.defaultBinaryMessenger
          .setMockMethodCallHandler(const MethodChannel('platform'), null);
    });
  }
}
