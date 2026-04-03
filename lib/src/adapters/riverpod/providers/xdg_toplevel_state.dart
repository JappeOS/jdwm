import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';
import 'package:jdwm/src/core/controllers/xdg_toplevel_controller.dart';
import 'package:jdwm/src/core/models/resize.dart';
import 'package:jdwm/src/core/models/toplevel_decoration.dart';
import 'package:jdwm/src/ui/common/xdg_toplevel_surface.dart';

part 'xdg_toplevel_state.freezed.dart';

part 'xdg_toplevel_state.g.dart';

final xdgToplevelControllerProvider = Provider<XdgToplevelController>(
  (ref) => const XdgToplevelController(),
);

@Riverpod(keepAlive: true)
XdgToplevelSurface xdgToplevelSurfaceWidget(Ref ref, int viewId) {
  return XdgToplevelSurface(
    key: ref.watch(
        xdgSurfaceStatesProvider(viewId).select((state) => state.widgetKey)),
    viewId: viewId,
  );
}

@freezed
abstract class XdgToplevelState with _$XdgToplevelState {
  const factory XdgToplevelState({
    required bool visible,
    required bool maximized,
    required Key virtualKeyboardKey,
    required FocusNode focusNode,
    required Object interactiveMoveRequested,
    required ResizeEdgeObject interactiveResizeRequested,
    required ToplevelDecoration decoration,
    required String title,
    required String appId,
  }) = _XdgToplevelState;
}

@Riverpod(keepAlive: true)
class XdgToplevelStates extends _$XdgToplevelStates {
  @override
  XdgToplevelState build(int viewId) {
    final focusNode = FocusNode();

    // Cannot access `state` inside onDispose.
    ref.onDispose(() {
      focusNode.dispose();
    });

    return _fromSnapshot(
      ref.read(xdgToplevelControllerProvider).initial(
            virtualKeyboardKey: GlobalKey(),
            focusNode: focusNode,
            interactiveMoveRequested: Object(),
            interactiveResizeRequested: ResizeEdgeObject(ResizeEdge.top),
            decoration: ToplevelDecoration.none,
          ),
    );
  }

  void requestVisible(bool value) {
    if (value != state.visible) {
      ref
          .read(platformApiProvider.notifier)
          .changeWindowVisibility(viewId, value);
    }
  }

  void requestMaximize(bool value) {
    ref.read(platformApiProvider.notifier).maximizeWindow(viewId, value);
  }

  void setWindowState({required bool visible, required bool maximized}) {
    var snapshot = _toSnapshot(state);
    snapshot =
        ref.read(xdgToplevelControllerProvider).setVisible(snapshot, visible);
    snapshot = ref
        .read(xdgToplevelControllerProvider)
        .setMaximized(snapshot, maximized);
    state = _fromSnapshot(snapshot);
  }

  void resize(int width, int height, {double? x, double? y}) {
    ref
        .read(platformApiProvider.notifier)
        .resizeWindow(viewId, width, height, x: x, y: y);
  }

  void requestInteractiveMove() {
    state = _fromSnapshot(
      ref
          .read(xdgToplevelControllerProvider)
          .requestInteractiveMove(_toSnapshot(state)),
    );
  }

  void requestInteractiveResize(ResizeEdge edge) {
    state = _fromSnapshot(
      ref
          .read(xdgToplevelControllerProvider)
          .requestInteractiveResize(_toSnapshot(state), ResizeEdgeObject(edge)),
    );
  }

  void setDecoration(ToplevelDecoration decoration) {
    state = _fromSnapshot(
      ref
          .read(xdgToplevelControllerProvider)
          .setDecoration(_toSnapshot(state), decoration),
    );
  }

  void setTitle(String title) {
    state = _fromSnapshot(
      ref
          .read(xdgToplevelControllerProvider)
          .setTitle(_toSnapshot(state), title),
    );
  }

  void setAppId(String appId) {
    state = _fromSnapshot(
      ref
          .read(xdgToplevelControllerProvider)
          .setAppId(_toSnapshot(state), appId),
    );
  }

  XdgToplevelSnapshot _toSnapshot(XdgToplevelState s) {
    return XdgToplevelSnapshot(
      visible: s.visible,
      maximized: s.maximized,
      virtualKeyboardKey: s.virtualKeyboardKey,
      focusNode: s.focusNode,
      interactiveMoveRequested: s.interactiveMoveRequested,
      interactiveResizeRequested: s.interactiveResizeRequested,
      decoration: s.decoration,
      title: s.title,
      appId: s.appId,
    );
  }

  XdgToplevelState _fromSnapshot(XdgToplevelSnapshot s) {
    return XdgToplevelState(
      visible: s.visible,
      maximized: s.maximized,
      virtualKeyboardKey: s.virtualKeyboardKey,
      focusNode: s.focusNode,
      interactiveMoveRequested: s.interactiveMoveRequested,
      interactiveResizeRequested: s.interactiveResizeRequested,
      decoration: s.decoration,
      title: s.title,
      appId: s.appId,
    );
  }
}
