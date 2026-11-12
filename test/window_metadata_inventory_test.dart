import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart' as riverpod;
import 'package:flutter_test/flutter_test.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;
import 'package:jdwm/src/window_manager.dart';
import 'package:jdwm/src/window_metadata.dart';
import 'package:jdwm/src/ui/common/xdg_toplevel_surface.dart';
import 'package:visibility_detector/visibility_detector.dart';

void main() {
  for (final protocol in ['xdg', 'xwayland']) {
    testWidgets(
        '$protocol metadata updates inventory and does not resurrect unmapped windows',
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
      final container = riverpod.ProviderScope.containerOf(
          tester.element(find.byType(XdgToplevelSurface)),
          listen: false);
      var notifications = 0;
      manager.addWindowStateListener(() => notifications++);
      await event('set_window_metadata', {
        'revision': 1,
        'wm_class': 'editor.exe',
        'wm_instance': 'wine',
        'icon_name': 'test-icon',
        'images': <Object>[],
      });
      expect(manager.getManagedWindowsByFocus().single.wmClass, 'editor.exe');
      expect(manager.getManagedWindowsByFocus().single.metadata.iconName,
          'test-icon');
      expect(notifications, greaterThan(0));
      await event('set_window_metadata', {'revision': 1, 'icon_name': 'stale'});
      expect(manager.getManagedWindowsByFocus().single.metadata.iconName,
          'test-icon');
      await event('set_window_metadata',
          {'revision': 2, 'icon_name': '', 'images': <Object>[]});
      expect(
          manager.getManagedWindowsByFocus().single.metadata.iconName, isEmpty);
      await event('unmap_toplevel_surface');
      await event('set_window_metadata', {'revision': 3, 'icon_name': 'late'});
      expect(container.read(windowMetadataProvider), isEmpty);
      expect(manager.getManagedWindowsByFocus(), isEmpty);
      await tester.pumpWidget(const SizedBox());
      tester.binding.defaultBinaryMessenger
          .setMockMethodCallHandler(const MethodChannel('platform'), null);
    });
  }
}
