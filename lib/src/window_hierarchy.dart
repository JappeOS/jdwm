import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'window.dart';
import 'window_entry.dart';

/// The single window stack. There is exactly one [WindowHierarchy] in the
/// widget tree (owned by [WindowManager]). It maintains the
/// authoritative list of open windows and their focus (z-order).
///
/// Per-monitor concerns (maximise rect, snap rect, coordinate conversion)
/// are handled by each [Window] reading its [WindowEntry.monitorId] and
/// looking up the corresponding [MonitorConfig] via [WindowManager].
class WindowHierarchy extends StatefulWidget {
  final Widget? alwaysOnTopWindow;

  const WindowHierarchy({
    GlobalKey<WindowHierarchyState>? key,
    this.alwaysOnTopWindow,
  }) : super(key: key);

  @override
  WindowHierarchyState createState() => WindowHierarchyState();

}

class WindowHierarchyState extends State<WindowHierarchy> {
  final List<WindowEntry> _entries = [];
  final List<WindowEntryId> _focusTree = [];
  final Map<WindowEntryId, GlobalKey> _windowKeys = {};

  void pushWindowEntry(WindowEntry entry) {
    _entries.add(entry);
    _focusTree.add(entry.id);
    _windowKeys[entry.id] = GlobalKey();
    if (mounted) {
      setState(() {});
    }
  }

  void popWindowEntry(WindowEntry entry) {
    _entries.removeWhere((e) => e.id == entry.id);
    _focusTree.remove(entry.id);
    _windowKeys.remove(entry.id);
    if (mounted) {
      setState(() {});
    }
  }

  void requestWindowFocus(WindowEntry entry) {
    _focusTree.remove(entry.id);
    _focusTree.add(entry.id);
    if (mounted) {
      setState(() {});
    }
  }

  List<WindowEntry> get windows => _entries;

  /// Windows ordered by focus (back to front).
  List<WindowEntry> get entriesByFocus {
    final result = <WindowEntry>[];
    for (final id in _focusTree) {
      final entry = _entries.where((e) => e.id == id).firstOrNull;
      if (entry != null) result.add(entry);
    }
    return result;
  }

  @override
  Widget build(BuildContext context) {
    return Provider<WindowHierarchyState>.value(
      value: this,
      updateShouldNotify: (previous, current) =>
          !listEquals(previous._entries, current._entries) ||
          !listEquals(previous._focusTree, current._focusTree),
      builder: (context, _) {
        return Stack(
          clipBehavior: Clip.none,
          children: [
            // Windows, back-to-front by focus
            ...entriesByFocus.map(
              (e) => Window(
                entry: e,
                key: _windowKeys[e.id]!,
              ),
            ),

            // Always-on-top overlay (e.g. a global toast layer)
            if (widget.alwaysOnTopWindow != null)
              Positioned.fill(
                child: IgnorePointer(
                  ignoring: false,
                  child: widget.alwaysOnTopWindow!,
                ),
              ),
          ],
        );
      },
    );
  }
}
