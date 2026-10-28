import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'window.dart';
import 'window_entry.dart';
import 'ui/common/window_occlusion_stack.dart';
import 'util/flutter_frame_trace.dart';

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
  final Map<WindowEntryId, WindowEntry> _entries = {};
  List<WindowEntry>? _ordered;
  final List<WindowEntryId> _focusTree = [];
  final Map<WindowEntryId, GlobalKey> _windowKeys = {};

  void pushWindowEntry(WindowEntry entry) {
    _entries[entry.id] = entry;
    _ordered = null;
    _focusTree.add(entry.id);
    _windowKeys[entry.id] = GlobalKey();
    if (mounted) {
      setState(() {});
    }
  }

  void popWindowEntry(WindowEntry entry) {
    _entries.remove(entry.id);
    _ordered = null;
    _focusTree.remove(entry.id);
    _windowKeys.remove(entry.id);
    if (mounted) {
      setState(() {});
    }
  }

  bool requestWindowFocus(WindowEntry entry) {
    if (!_entries.containsKey(entry.id) || _focusTree.lastOrNull == entry.id) {
      return false;
    }
    _ordered = null;
    FlutterFrameTrace.focusChanged();
    _focusTree.remove(entry.id);
    _focusTree.add(entry.id);
    if (mounted) {
      setState(() {});
    }
    return true;
  }

  List<WindowEntry> get windows => List.unmodifiable(_entries.values);

  WindowEntry? get focusedEntry => _entries[_focusTree.lastOrNull];

  /// Cached immutable snapshot; only membership and z-order invalidate it.
  List<WindowEntry> get entriesByFocus => _ordered ??= List.unmodifiable(
      _focusTree.map((id) => _entries[id]).whereType<WindowEntry>());

  @override
  Widget build(BuildContext context) {
    return Provider<WindowHierarchyState>.value(
      value: this,
      updateShouldNotify: (previous, current) => true,
      builder: (context, _) {
        final ordered = entriesByFocus;
        return WindowOcclusionStack(
          contentKeys: [
            for (final entry in ordered) entry.repaintBoundaryKey,
            if (widget.alwaysOnTopWindow != null) null,
          ],
          children: [
            // Windows, back-to-front by focus
            ...ordered.map(
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
