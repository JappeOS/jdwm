import 'package:flutter/material.dart';

import 'window_manager.dart';

/// A lightweight widget positioned at a monitor's bounds. It exposes its
/// [MonitorConfig] to descendants via [of] and provides a stable [GlobalKey]
/// whose [RenderBox] can be used for global <-> local coordinate conversion.
///
/// No window state lives here, it is purely a spatial anchor.
class MonitorRegion extends StatelessWidget {
  final MonitorConfig config;
  final GlobalKey regionKey;
  final Widget child;

  const MonitorRegion({
    super.key,
    required this.config,
    required this.regionKey,
    required this.child,
  });

  /// Look up the [MonitorConfig] for the nearest ancestor [MonitorRegion].
  static MonitorConfig? of(BuildContext context) {
    return context
        .dependOnInheritedWidgetOfExactType<_MonitorRegionInherited>()
        ?.config;
  }

  @override
  Widget build(BuildContext context) {
    return _MonitorRegionInherited(
      config: config,
      child: SizedBox.fromSize(
        key: regionKey,
        size: config.bounds.size,
        child: child,
      ),
    );
  }
}

class _MonitorRegionInherited extends InheritedWidget {
  final MonitorConfig config;

  const _MonitorRegionInherited({
    required this.config,
    required super.child,
  });

  @override
  bool updateShouldNotify(_MonitorRegionInherited oldWidget) =>
      config != oldWidget.config;
}