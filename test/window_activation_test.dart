import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart' as riverpod;
import 'package:flutter_test/flutter_test.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;
import 'package:jdwm/src/window_manager.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/ui/common/xdg_toplevel_surface.dart';
import 'package:visibility_detector/visibility_detector.dart';

void main() {
  for (final protocol in ['xdg', 'xwayland']) {
    testWidgets('$protocol activation restores a minimized topmost window',
        (tester) async {
      VisibilityDetectorController.instance.updateInterval = Duration.zero;
      final calls = <MethodCall>[];
      tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
          const MethodChannel('platform'), (call) async {
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
            MonitorConfig(
                id: 'main', bounds: bounds, margin: EdgeInsets.all(30)),
          ]),
        ),
      ));
      Future<void> event(String method,
          [Map<String, Object> data = const {}]) async {
        final done = Completer<void>();
        tester.binding.channelBuffers.push(
            'platform',
            const StandardMethodCodec().encodeMethodCall(MethodCall(method, {
              'view_id': 1,
              'protocol': protocol,
              ...data,
            })),
            (_) => done.complete());
        await done.future;
        await tester.pump();
        await tester.pump();
      }

      await event(
          'commit_toplevel_surface', {'has_decoration': true, 'decoration': 2});
      await event('map_toplevel_surface');
      final manager = key.currentState!;
      final window = manager.getAllWindows().single;
      final container = riverpod.ProviderScope.containerOf(
          tester.element(find.byType(XdgToplevelSurface)),
          listen: false);
      final original = window.windowRect;
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
