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

WindowEntry entry() => WindowEntry(
      icon: const AssetImage('unused'),
      initialSize: const Size(200, 180),
      content: const SizedBox.expand(),
    );

void main() {
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
