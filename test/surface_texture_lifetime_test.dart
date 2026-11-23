import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/ui/common/surface.dart';
import 'package:jdwm/src/ui/common/popup_stack.dart';
import 'package:jdwm/src/core/models/xdg_surface_role.dart';

void main() {
  testWidgets('unregister after native destroy and final widget detach',
      (tester) async {
    const channel = MethodChannel('platform');
    final releases = <int>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(channel,
        (call) async {
      if (call.method == 'unregister_view_texture') {
        releases.add(call.arguments as int);
      }
      return null;
    });
    final container = ProviderContainer();
    addTearDown(container.dispose);
    container.read(platformApiProvider.notifier).init();

    Future<void> show(int? id) => tester.pumpWidget(UncontrolledProviderScope(
          container: container,
          child: Directionality(
            textDirection: TextDirection.ltr,
            child: id == null ? const SizedBox() : Surface(viewId: id),
          ),
        ));

    Future<void> event(String method, Object arguments) async {
      final done = Completer<void>();
      tester.binding.channelBuffers.push(
        channel.name,
        const StandardMethodCodec().encodeMethodCall(MethodCall(method, arguments)),
        (_) => done.complete(),
      );
      await done.future;
    }

    await show(1);
    await show(null);
    await show(1); // Remapping the same live surface must keep its texture.
    expect(releases, isEmpty);
    await event('texture_destroyed', 1);
    expect(releases, isEmpty); // Still mounted during the close animation.
    await show(2);
    expect(releases, [1]);
    await show(null);
    await event('texture_destroyed', 2);
    await event('texture_destroyed', 3); // Registered but never displayed.
    expect(releases, [1, 2, 3]);

    // A client can map and close before the first buffer commit reaches Dart.
    final popup = {'view_id': 4, 'role': XdgSurfaceRole.popup.index};
    await event('map_xdg_surface', popup);
    expect(container.read(popupStackChildrenProvider), contains(4));
    await event('unmap_xdg_surface', popup);
    await tester.pump();
    expect(container.read(popupStackChildrenProvider), isNot(contains(4)));
  });
}
