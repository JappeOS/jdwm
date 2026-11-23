import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/jdwm.dart';
import 'package:jdwm/src/adapters/riverpod/providers/subsurface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/core/models/surface_role.dart';
import 'package:jdwm/src/core/models/xdg_surface_role.dart';
import 'package:jdwm/src/ui/common/view_input_listener.dart';

void _commit(
  ProviderContainer container,
  int id,
  Size size, {
  int? textureId,
  List<int> below = const [],
  List<int> above = const [],
}) {
  container.read(surfaceStatesProvider(id).notifier).commit(
        role: id == 1 ? SurfaceRole.xdgSurface : SurfaceRole.subsurface,
        textureId: textureId ?? id,
        surfacePosition: Offset.zero,
        surfaceSize: size,
        scale: 2,
        bufferSourceBox: Offset.zero & (size * 2),
        bufferSize: size * 2,
        inputRegion: Offset.zero & size,
        subsurfacesBelow: below,
        subsurfacesAbove: above,
      );
}

void main() {
  testWidgets(
      'previews share live surface textures and leave input to the tile',
      (tester) async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final calls = <MethodCall>[];
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('platform'),
      (call) async {
        calls.add(call);
        return null;
      },
    );
    const size = Size(220, 120);
    _commit(container, 1, size, below: [2], above: [3]);
    for (final id in [2, 3]) {
      _commit(container, id, const Size(20, 20));
      container.read(subsurfaceStatesProvider(id).notifier)
        ..map(true)
        ..commit(position: const Offset(30, 30));
    }
    container.read(xdgSurfaceStatesProvider(1).notifier).commit(
          role: XdgSurfaceRole.toplevel,
          visibleBounds: const Rect.fromLTWH(10, 10, 200, 100),
        );
    final windows = container.read(mappedWindowListProvider.notifier);
    windows.add(1);

    const first = WindowPreview(viewId: 1, width: 100, height: 100);
    const second = WindowPreview(viewId: 1, width: 80, height: 50);
    int taps = 0;
    Future<void> show({bool client = true, bool extra = true}) =>
        tester.pumpWidget(UncontrolledProviderScope(
          container: container,
          child: Directionality(
            textDirection: TextDirection.ltr,
            child: Row(children: [
              if (client) container.read(surfaceWidgetProvider(1)),
              GestureDetector(
                  behavior: HitTestBehavior.opaque,
                  onTap: () => taps++,
                  child: first),
              if (extra) second,
            ]),
          ),
        ));
    final preview = find.byWidget(first);
    final textures =
        find.descendant(of: preview, matching: find.byType(Texture));
    List<int> ids() =>
        tester.widgetList<Texture>(textures).map((t) => t.textureId).toList();

    await show();
    expect(tester.takeException(), isNull);
    expect(find.byType(Texture), findsNWidgets(9));
    expect(ids(), [2, 1, 3]);
    expect(
        find.descendant(of: preview, matching: find.byType(ViewInputListener)),
        findsNothing);
    expect(tester.getSize(preview), const Size(100, 100));
    // Contain centers the 2:1 window in the square tile. Its ten-pixel client
    // margins are outside the crop, and surface coordinates scale by one half.
    final origin = tester.getTopLeft(preview);
    expect(tester.getTopLeft(textures.at(1)) - origin, const Offset(-5, 20));
    expect(tester.getTopLeft(textures.at(2)) - origin, const Offset(10, 35));
    calls.clear();
    await tester.tapAt(tester.getCenter(preview));
    await tester.pump();
    expect(taps, 1);
    expect(calls, isEmpty);

    _commit(container, 1, size, textureId: 42, below: [2], above: [3]);
    container.read(subsurfaceStatesProvider(2).notifier).map(false);
    await tester.pump();
    expect(ids(), [42, 3]);

    // The main window can disappear while several previews still paint it.
    container.read(platformApiProvider.notifier).surfaceTextures.destroy(1);
    await show(client: false);
    await show(client: false, extra: false);
    expect(calls.where((c) => c.method == 'unregister_view_texture'), isEmpty);
    windows.remove(1);
    await tester.pump();
    expect(textures, findsNothing);
    expect(tester.getSize(preview), const Size(100, 100));
    expect(
        calls
            .where((c) => c.method == 'unregister_view_texture')
            .map((c) => c.arguments),
        [1]);
  });
}
