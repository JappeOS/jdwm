import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart' as riverpod;
import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart' as shadcn;
import 'package:jdwm/src/window_entry.dart';
import 'package:jdwm/src/window_hierarchy.dart';
import 'package:jdwm/src/window_manager.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/ui/common/xdg_toplevel_surface.dart';
import 'package:visibility_detector/visibility_detector.dart';
import 'package:jdwm/src/backend/platform_api.dart';

WindowEntry entry() => WindowEntry(
      icon: const AssetImage('unused'),
      initialSize: const Size(200, 180),
      content: const SizedBox.expand(),
    );

void main() {
  for (final protocol in ['xdg', 'xwayland']) {
    testWidgets('$protocol fullscreen events resize to full monitor bounds',
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
      final api = container.read(platformApiProvider.notifier);
      window.windowRect = const Rect.fromLTWH(150, 60, 200, 180);
      window.backendContentInsetTop = 25;
      final original = window.windowRect;
      await event('set_toplevel_state',
          {'visible': true, 'maximized': false, 'fullscreen': true});
      expect(window.fullscreen, isTrue);
      expect(manager.getManagedWindowsByFocus().single.fullscreen, isTrue);
      expect(api.currentOpenWindows.single.fullscreen, isTrue);
      expect(window.usesToolbar, isFalse);
      expect(manager.targetRectForWindow(window), bounds);
      final resize = calls.lastWhere((call) => call.method == 'resize_window');
      expect(resize.arguments,
          {'view_id': 1, 'x': 100.0, 'y': 0.0, 'width': 600, 'height': 500});
      // Subsequent commits and legacy state payloads must not clear fullscreen.
      await event('commit_toplevel_surface', {'width': 600, 'height': 500});
      await event('set_toplevel_state', {'visible': true, 'maximized': false});
      expect(window.fullscreen, isTrue);
      await event('set_toplevel_state',
          {'visible': true, 'maximized': false, 'fullscreen': false});
      expect(window.fullscreen, isFalse);
      expect(manager.getManagedWindowsByFocus().single.fullscreen, isFalse);
      expect(api.currentOpenWindows.single.fullscreen, isFalse);
      expect(window.usesToolbar, isTrue);
      expect(window.windowRect, original);
      await tester.pumpWidget(const SizedBox());
      tester.binding.defaultBinaryMessenger
          .setMockMethodCallHandler(const MethodChannel('platform'), null);
    });
  }

  testWidgets(
      'fullscreen fills monitor bounds and restores window chrome and geometry',
      (tester) async {
    final key = GlobalKey<WindowManagerState>();
    const full = Rect.fromLTWH(400, 0, 400, 500);
    await tester.pumpWidget(shadcn.Theme(
        data: const shadcn.ThemeData(),
        child: Directionality(
            textDirection: TextDirection.ltr,
            child: WindowManager(
                key: key,
                enableZenithBackend: false,
                monitors: const [
                  MonitorConfig(
                      id: 'left', bounds: Rect.fromLTWH(0, 0, 400, 500)),
                  MonitorConfig(
                      id: 'right', bounds: full, margin: EdgeInsets.all(30)),
                ]))));
    final manager = key.currentState!;
    final contentKey = GlobalKey();
    final window = WindowEntry(
        icon: const AssetImage('unused'),
        content: SizedBox.expand(key: contentKey));
    window.toolbar = const SizedBox(key: ValueKey('header'), height: 25);
    manager.pushWindow(window, monitorId: 'right');
    window.windowRect = const Rect.fromLTWH(430, 40, 200, 180);
    final original = window.windowRect;
    await tester.pump();
    window.fullscreen = true;
    await tester.pump();
    expect(manager.targetRectForWindow(window), full);
    expect(tester.getRect(find.byKey(contentKey)), full);
    expect(find.byKey(const ValueKey('header')), findsNothing);
    window.fullscreen =
        true; // Duplicate requests cannot overwrite restoration.
    window.fullscreen = false;
    await tester.pump();
    expect(window.windowRect, original);
    expect(find.byKey(const ValueKey('header')), findsOneWidget);
    window.maximized = true;
    window.fullscreen = true;
    await tester.pump();
    expect(manager.targetRectForWindow(window), full);
    window.fullscreen = false;
    await tester.pump();
    expect(window.maximized, isTrue);
    expect(manager.targetRectForWindow(window), full.deflate(30));
    await tester.pumpWidget(const SizedBox());
  });

  testWidgets('backend focus activates once and restores after an overlay',
      (tester) async {
    VisibilityDetectorController.instance.updateInterval = Duration.zero;
    final calls = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
        const MethodChannel('platform'), (call) async {
      calls.add(call);
      return null;
    });
    final managerKey = GlobalKey<WindowManagerState>();
    final overlayFocus = FocusNode();
    await tester.pumpWidget(shadcn.Theme(
      data: const shadcn.ThemeData(),
      child: Directionality(
        textDirection: TextDirection.ltr,
        child: FocusScope(
            child: Stack(children: [
          WindowManager(key: managerKey),
          Focus(focusNode: overlayFocus, child: const SizedBox()),
        ])),
      ),
    ));
    Future<void> nativeEvent(String method, int id) async {
      final done = Completer<void>();
      tester.binding.channelBuffers.push(
          'platform',
          const StandardMethodCodec().encodeMethodCall(MethodCall(method, {
            'view_id': id,
            'protocol': 'xdg',
            'has_decoration': true,
            'decoration': 2
          })),
          (_) => done.complete());
      await done.future;
      await tester.pump();
      await tester.pump();
    }

    await nativeEvent('commit_toplevel_surface', 1);
    await nativeEvent('map_toplevel_surface', 1);
    final manager = managerKey.currentState!;
    final window = manager.getAllWindows().single;
    final container = riverpod.ProviderScope.containerOf(
        tester.element(find.byType(XdgToplevelSurface)),
        listen: false);
    final node = container.read(xdgToplevelStatesProvider(1)).focusNode;
    expect(node.hasFocus, isTrue);
    List<List<dynamic>> activations() => calls
        .where((c) => c.method == 'activate_window')
        .map((c) => c.arguments as List<dynamic>)
        .toList();
    expect(activations(), [
      [1, true]
    ]);
    for (var i = 0; i < 10; i++) {
      manager.requestFocus(window);
      await tester.pump();
    }
    expect(activations(), [
      [1, true]
    ]);
    overlayFocus.requestFocus();
    await tester.pump();
    expect(node.hasFocus, isFalse);
    manager.requestFocus(window);
    await tester.pump();
    expect(node.hasFocus, isTrue);
    expect(activations(), [
      [1, true],
      [1, false],
      [1, true]
    ]);
    await nativeEvent('commit_toplevel_surface', 2);
    await nativeEvent('map_toplevel_surface', 2);
    expect(container.read(xdgToplevelStatesProvider(2)).focusNode.hasFocus,
        isTrue);
    expect(node.hasFocus, isFalse);
    await nativeEvent('unmap_toplevel_surface', 2);
    expect(node.hasFocus, isTrue);
    await tester.pumpWidget(const SizedBox());
    overlayFocus.dispose();
    tester.binding.defaultBinaryMessenger
        .setMockMethodCallHandler(const MethodChannel('platform'), null);
  });

  test('unchanged state and pixel-rounded geometry do not notify', () {
    final window = entry();
    var notifications = 0;
    window.addListener(() => notifications++);
    window.windowRect = window.windowRect.translate(0.1, 0.1);
    window.monitorId = window.monitorId;
    window.maximized = false;
    window.minimized = false;
    window.windowDock = window.windowDock;
    window.title = window.title;
    window.icon = window.icon;
    window.chromeMode = window.chromeMode;
    window.toolbar = window.toolbar;
    expect(notifications, 0);
    window.windowRect = window.windowRect.translate(1, 0);
    expect(notifications, 1);
    window.dispose();
  });

  testWidgets('repeated focus and cursor moves retain stationary windows',
      (tester) async {
    final managerKey = GlobalKey<WindowManagerState>();
    await tester.pumpWidget(shadcn.Theme(
      data: const shadcn.ThemeData(),
      child: Directionality(
        textDirection: TextDirection.ltr,
        child: WindowManager(key: managerKey, enableZenithBackend: false),
      ),
    ));
    final manager = managerKey.currentState!;
    final first = entry(), second = entry();
    var builds = 0;
    first.toolbar = Builder(builder: (context) {
      context.watch<WindowHierarchyState>();
      builds++;
      return const SizedBox(height: 20);
    });
    second.toolbar = const SizedBox(height: 20);
    manager.pushWindow(first);
    manager.pushWindow(second);
    await tester.pump();
    final hierarchy =
        tester.state<WindowHierarchyState>(find.byType(WindowHierarchy));
    final order = hierarchy.entriesByFocus;
    final initialBuilds = builds;
    for (var i = 0; i < 10; i++) {
      manager.requestFocus(second);
      manager.setClientCursor(
          SystemMouseCursors.grabbing, Offset(100.0 + i, 50));
      second.windowRect = second.windowRect.translate(1, 0);
      await tester.pump();
    }
    expect(hierarchy.entriesByFocus, same(order));
    expect(builds, initialBuilds);
    manager.requestFocus(first);
    await tester.pump();
    expect(hierarchy.focusedEntry, first);
    expect(hierarchy.entriesByFocus, [second, first]);
    expect(builds, greaterThan(initialBuilds));
    manager.popWindow(first);
    manager.endClientCursor();
    await tester.pump();
    expect(hierarchy.focusedEntry, second);
    await tester.pumpWidget(const SizedBox());
    first.dispose();
    second.dispose();
  });
}
