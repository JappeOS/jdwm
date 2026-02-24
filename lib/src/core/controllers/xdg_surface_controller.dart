import 'package:flutter/material.dart';
import 'package:jdwm/src/core/models/xdg_surface_role.dart';

class XdgSurfaceSnapshot {
  final XdgSurfaceRole role;
  final Rect visibleBounds;
  final GlobalKey widgetKey;
  final List<int> popups;

  const XdgSurfaceSnapshot({
    required this.role,
    required this.visibleBounds,
    required this.widgetKey,
    required this.popups,
  });
}

class XdgSurfaceController {
  const XdgSurfaceController();

  XdgSurfaceSnapshot initial({
    required XdgSurfaceRole role,
    required GlobalKey widgetKey,
  }) {
    return XdgSurfaceSnapshot(
      role: role,
      visibleBounds: Rect.zero,
      widgetKey: widgetKey,
      popups: const <int>[],
    );
  }

  XdgSurfaceSnapshot commit(
    XdgSurfaceSnapshot state, {
    required XdgSurfaceRole role,
    required Rect visibleBounds,
  }) {
    return XdgSurfaceSnapshot(
      role: role,
      visibleBounds: visibleBounds,
      widgetKey: state.widgetKey,
      popups: state.popups,
    );
  }

  XdgSurfaceSnapshot addPopup(XdgSurfaceSnapshot state, int viewId) {
    return XdgSurfaceSnapshot(
      role: state.role,
      visibleBounds: state.visibleBounds,
      widgetKey: state.widgetKey,
      popups: <int>[...state.popups, viewId],
    );
  }

  XdgSurfaceSnapshot removePopup(XdgSurfaceSnapshot state, int viewId) {
    return XdgSurfaceSnapshot(
      role: state.role,
      visibleBounds: state.visibleBounds,
      widgetKey: state.widgetKey,
      popups: <int>[
        for (final id in state.popups)
          if (id != viewId) id,
      ],
    );
  }
}
