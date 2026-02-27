import 'package:flutter/material.dart';

class XdgPopupSnapshot {
  final int parentViewId;
  final Offset position;
  final GlobalKey animationsKey;
  final bool isClosing;

  const XdgPopupSnapshot({
    required this.parentViewId,
    required this.position,
    required this.animationsKey,
    required this.isClosing,
  });
}

class XdgPopupController {
  const XdgPopupController();

  XdgPopupSnapshot initial({required GlobalKey animationsKey}) {
    return XdgPopupSnapshot(
      parentViewId: -1,
      position: Offset.zero,
      animationsKey: animationsKey,
      isClosing: false,
    );
  }

  XdgPopupSnapshot commit(
    XdgPopupSnapshot state, {
    required int parentViewId,
    required Offset position,
  }) {
    return XdgPopupSnapshot(
      parentViewId: parentViewId,
      position: position,
      animationsKey: state.animationsKey,
      // A new commit while mapped means the popup is active again.
      isClosing: false,
    );
  }

  XdgPopupSnapshot setParentViewId(XdgPopupSnapshot state, int value) {
    return XdgPopupSnapshot(
      parentViewId: value,
      position: state.position,
      animationsKey: state.animationsKey,
      isClosing: state.isClosing,
    );
  }

  XdgPopupSnapshot setPosition(XdgPopupSnapshot state, Offset value) {
    return XdgPopupSnapshot(
      parentViewId: state.parentViewId,
      position: value,
      animationsKey: state.animationsKey,
      isClosing: state.isClosing,
    );
  }

  XdgPopupSnapshot startClosing(XdgPopupSnapshot state) {
    return XdgPopupSnapshot(
      parentViewId: state.parentViewId,
      position: state.position,
      animationsKey: state.animationsKey,
      isClosing: true,
    );
  }
}
