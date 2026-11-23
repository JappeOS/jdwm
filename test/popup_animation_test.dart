import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_popup_state.dart';
import 'package:jdwm/src/core/models/surface_role.dart';
import 'package:jdwm/src/ui/common/popup_stack.dart';

void main() {
  testWidgets('popup follows its cropped parent in logical surface coordinates',
      (tester) async {
    final container = ProviderContainer();
    addTearDown(container.dispose);
    const parent = 3, popup = 4;
    for (final id in [parent, popup]) {
      final size = id == parent ? const Size(200, 120) : const Size(40, 20);
      container.read(surfaceStatesProvider(id).notifier).commit(
        role: SurfaceRole.xdgSurface,
        textureId: id,
        surfacePosition: Offset.zero,
        surfaceSize: size,
        scale: 2,
        bufferSize: Size(size.width * 2 + 30, size.height * 2 + 20),
        bufferSourceBox: const Offset(30, 20) & (size * 2),
        inputRegion: Offset.zero & size,
        subsurfacesBelow: [],
        subsurfacesAbove: [],
      );
    }
    final owner = container.read(xdgPopupStatesProvider(popup).notifier);
    owner.commit(parentViewId: parent, position: const Offset(20, 25));
    owner.map();
    Future<void> show(double x) => tester.pumpWidget(UncontrolledProviderScope(
          container: container,
          child: Directionality(
            textDirection: TextDirection.ltr,
            child: Stack(children: [
              Positioned(
                  left: x,
                  top: 70,
                  child: container.read(surfaceWidgetProvider(parent))),
              const PopupStack(),
            ]),
          ),
        ));
    await show(100);
    await tester.pumpAndSettle();
    final child = find.byWidget(container.read(surfaceWidgetProvider(popup)));
    expect(tester.getTopLeft(child), const Offset(120, 95));
    await show(200);
    expect(tester.getTopLeft(child), const Offset(220, 95));
    expect(child.hitTestable(), findsOneWidget);
    owner.unmap();
    await tester.pumpWidget(const SizedBox());
    await tester.pump();
    expect(container.read(popupStackChildrenProvider), isEmpty);
  });
  testWidgets(
    'popup with delayed geometry plays its opening and closing slide',
    (tester) async {
      const id = 9;
      final container = ProviderContainer();
      addTearDown(container.dispose);
      container.read(popupStackChildrenProvider.notifier).add(id);
      await tester.pumpWidget(
        UncontrolledProviderScope(
          container: container,
          child: const Directionality(
            textDirection: TextDirection.ltr,
            child: PopupStack(),
          ),
        ),
      );
      final fade = find.byType(FadeTransition);
      await tester.pump(const Duration(milliseconds: 300));
      expect(fade, findsNothing);

      void setHeight(double height) {
        final bounds = Rect.fromLTWH(0, 0, 100, height);
        container.read(surfaceStatesProvider(id).notifier).commit(
          role: SurfaceRole.xdgSurface,
          textureId: id,
          surfacePosition: Offset.zero,
          surfaceSize: bounds.size,
          scale: 1,
          bufferSourceBox: bounds,
          bufferSize: bounds.size,
          inputRegion: bounds,
          subsurfacesAbove: [],
          subsurfacesBelow: [],
        );
      }

      double slide() => tester
          .widget<Transform>(
            find.descendant(of: fade, matching: find.byType(Transform)).first,
          )
          .transform
          .getTranslation()
          .y;

      setHeight(100);
      await tester.pump();
      expect(slide(), -10);
      await tester.pump(const Duration(milliseconds: 100));
      final midpoint = slide();
      expect(midpoint, allOf(greaterThan(-10), lessThan(0)));
      setHeight(240);
      await tester.pump();
      expect(slide(), midpoint);
      await tester.pump(const Duration(milliseconds: 100));
      expect(slide(), 0);

      container.read(xdgPopupStatesProvider(id).notifier).unmap();
      await tester.pump();
      await tester.pump(const Duration(milliseconds: 50));
      expect(slide(), allOf(greaterThan(-10), lessThan(0)));
      final popup = container.read(xdgPopupStatesProvider(id).notifier);
      popup.map();
      await tester.pumpAndSettle();
      expect(slide(), 0);
      expect(container.read(popupStackChildrenProvider), contains(id));
      popup.unmap();
      await tester.pumpAndSettle();
      expect(fade, findsNothing);
    },
  );
}
