import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/ui/common/popup.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';
import 'package:jdwm/src/core/controllers/xdg_popup_controller.dart';

part 'xdg_popup_state.freezed.dart';

part 'xdg_popup_state.g.dart';

final xdgPopupControllerProvider = Provider<XdgPopupController>(
  (ref) => const XdgPopupController(),
);

@Riverpod(keepAlive: true)
Popup popupWidget(Ref ref, int viewId) {
  return Popup(
    key: ref.watch(xdgSurfaceStatesProvider(viewId).select((state) => state.widgetKey)),
    viewId: viewId,
  );
}

@freezed
abstract class XdgPopupState with _$XdgPopupState {
  const factory XdgPopupState({
    required int parentViewId,
    required Offset position,
    required GlobalKey<AnimationsState> animationsKey,
    required bool isClosing,
  }) = _XdgPopupState;
}

@Riverpod(keepAlive: true)
class XdgPopupStates extends _$XdgPopupStates {
  @override
  XdgPopupState build(int viewId) {
    return _fromSnapshot(
      ref.read(xdgPopupControllerProvider).initial(
            animationsKey: GlobalKey<AnimationsState>(),
          ),
    );
  }

  void commit({
    required int parentViewId,
    required Offset position,
  }) {
    state = _fromSnapshot(
      ref
          .read(xdgPopupControllerProvider)
          .commit(_toSnapshot(state), parentViewId: parentViewId, position: position),
    );
  }

  set parentViewId(int value) {
    state = _fromSnapshot(
      ref.read(xdgPopupControllerProvider).setParentViewId(_toSnapshot(state), value),
    );
  }

  set position(Offset value) {
    state = _fromSnapshot(
      ref.read(xdgPopupControllerProvider).setPosition(_toSnapshot(state), value),
    );
  }

  FutureOr animateClosing() {
    state = _fromSnapshot(
      ref.read(xdgPopupControllerProvider).startClosing(_toSnapshot(state)),
    );
    return state.animationsKey.currentState?.controller.reverse();
  }

  XdgPopupSnapshot _toSnapshot(XdgPopupState s) {
    return XdgPopupSnapshot(
      parentViewId: s.parentViewId,
      position: s.position,
      animationsKey: s.animationsKey,
      isClosing: s.isClosing,
    );
  }

  XdgPopupState _fromSnapshot(XdgPopupSnapshot s) {
    return XdgPopupState(
      parentViewId: s.parentViewId,
      position: s.position,
      animationsKey: s.animationsKey as GlobalKey<AnimationsState>,
      isClosing: s.isClosing,
    );
  }
}
