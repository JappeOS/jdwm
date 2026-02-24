import 'package:flutter/material.dart';
import 'package:jdwm/src/core/models/resize.dart';
import 'package:jdwm/src/core/models/toplevel_decoration.dart';

class XdgToplevelSnapshot {
  final bool visible;
  final bool maximized;
  final Key virtualKeyboardKey;
  final FocusNode focusNode;
  final Object interactiveMoveRequested;
  final ResizeEdgeObject interactiveResizeRequested;
  final ToplevelDecoration decoration;
  final String title;
  final String appId;

  const XdgToplevelSnapshot({
    required this.visible,
    required this.maximized,
    required this.virtualKeyboardKey,
    required this.focusNode,
    required this.interactiveMoveRequested,
    required this.interactiveResizeRequested,
    required this.decoration,
    required this.title,
    required this.appId,
  });
}

class XdgToplevelController {
  const XdgToplevelController();

  XdgToplevelSnapshot initial({
    required Key virtualKeyboardKey,
    required FocusNode focusNode,
    required Object interactiveMoveRequested,
    required ResizeEdgeObject interactiveResizeRequested,
    required ToplevelDecoration decoration,
  }) {
    return XdgToplevelSnapshot(
      visible: true,
      maximized: false,
      virtualKeyboardKey: virtualKeyboardKey,
      focusNode: focusNode,
      interactiveMoveRequested: interactiveMoveRequested,
      interactiveResizeRequested: interactiveResizeRequested,
      decoration: decoration,
      title: '',
      appId: '',
    );
  }

  XdgToplevelSnapshot setVisible(XdgToplevelSnapshot state, bool value) {
    return XdgToplevelSnapshot(
      visible: value,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: state.decoration,
      title: state.title,
      appId: state.appId,
    );
  }

  XdgToplevelSnapshot requestInteractiveMove(XdgToplevelSnapshot state) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: Object(),
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: state.decoration,
      title: state.title,
      appId: state.appId,
    );
  }

  XdgToplevelSnapshot requestInteractiveResize(
    XdgToplevelSnapshot state,
    ResizeEdgeObject resizeRequest,
  ) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: resizeRequest,
      decoration: state.decoration,
      title: state.title,
      appId: state.appId,
    );
  }

  XdgToplevelSnapshot setDecoration(
    XdgToplevelSnapshot state,
    ToplevelDecoration decoration,
  ) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: decoration,
      title: state.title,
      appId: state.appId,
    );
  }

  XdgToplevelSnapshot setTitle(XdgToplevelSnapshot state, String title) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: state.decoration,
      title: title,
      appId: state.appId,
    );
  }

  XdgToplevelSnapshot setAppId(XdgToplevelSnapshot state, String appId) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: state.maximized,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: state.decoration,
      title: state.title,
      appId: appId,
    );
  }

  XdgToplevelSnapshot setMaximized(XdgToplevelSnapshot state, bool value) {
    return XdgToplevelSnapshot(
      visible: state.visible,
      maximized: value,
      virtualKeyboardKey: state.virtualKeyboardKey,
      focusNode: state.focusNode,
      interactiveMoveRequested: state.interactiveMoveRequested,
      interactiveResizeRequested: state.interactiveResizeRequested,
      decoration: state.decoration,
      title: state.title,
      appId: state.appId,
    );
  }
}
