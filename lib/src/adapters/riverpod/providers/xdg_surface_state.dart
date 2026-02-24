import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_popup_state.dart';
import 'package:jdwm/src/core/controllers/xdg_surface_controller.dart';
import 'package:jdwm/src/core/models/xdg_surface_role.dart';

part 'xdg_surface_state.freezed.dart';

part 'xdg_surface_state.g.dart';

final xdgSurfaceControllerProvider = Provider<XdgSurfaceController>(
  (ref) => const XdgSurfaceController(),
);

@freezed
abstract class XdgSurfaceState with _$XdgSurfaceState {
  const factory XdgSurfaceState({
    required XdgSurfaceRole role,
    required Rect visibleBounds,
    required GlobalKey widgetKey,
    required List<int> popups,
  }) = _XdgSurfaceState;
}

@Riverpod(keepAlive: true)
class XdgSurfaceStates extends _$XdgSurfaceStates {
  @override
  XdgSurfaceState build(int viewId) {
    return _fromSnapshot(
      ref.read(xdgSurfaceControllerProvider).initial(
            role: XdgSurfaceRole.none,
            widgetKey: GlobalKey(),
          ),
    );
  }

  void commit({
    required XdgSurfaceRole role,
    required Rect visibleBounds,
  }) {
    state = _fromSnapshot(
      ref.read(xdgSurfaceControllerProvider).commit(
            _toSnapshot(state),
            role: role,
            visibleBounds: visibleBounds,
          ),
    );
  }

  void addPopup(int viewId) {
    state = _fromSnapshot(
      ref.read(xdgSurfaceControllerProvider).addPopup(_toSnapshot(state), viewId),
    );
    ref.read(xdgPopupStatesProvider(viewId).notifier).parentViewId = this.viewId;
  }

  void removePopup(int viewId) {
    state = _fromSnapshot(
      ref.read(xdgSurfaceControllerProvider).removePopup(_toSnapshot(state), viewId),
    );
  }

  XdgSurfaceSnapshot _toSnapshot(XdgSurfaceState s) {
    return XdgSurfaceSnapshot(
      role: s.role,
      visibleBounds: s.visibleBounds,
      widgetKey: s.widgetKey,
      popups: s.popups,
    );
  }

  XdgSurfaceState _fromSnapshot(XdgSurfaceSnapshot s) {
    return XdgSurfaceState(
      role: s.role,
      visibleBounds: s.visibleBounds,
      widgetKey: s.widgetKey,
      popups: s.popups,
    );
  }
}
