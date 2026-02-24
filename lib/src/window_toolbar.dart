import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:provider/provider.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_toplevel_state.dart';
import 'package:jdwm/src/backend/platform_api.dart';

import 'window.dart';
import 'window_entry.dart';
import 'window_hierarchy.dart';
import 'window_manager.dart';

class DefaultWindowToolbar extends StatelessWidget {
  const DefaultWindowToolbar({super.key});

  ProviderContainer? _backendContainer(BuildContext context) {
    try {
      return ProviderScope.containerOf(context, listen: false);
    } catch (_) {
      return null;
    }
  }

  @override
  Widget build(BuildContext context) {
    final entry = context.watch<WindowEntry>();
    final callbacks = TitlebarDragCallbacks.of(context);

    final entriesByFocus = context.read<WindowHierarchyState>().entriesByFocus;
    final isFocused
        = entriesByFocus.isNotEmpty && entriesByFocus.last == entry;

    void onMaximizeOrRestore() {
      final manager = WindowManager.of(context);
      if (manager != null) {
        manager.requestFocus(entry);
      } else {
        context
            .read<WindowHierarchyState>()
            .requestWindowFocus(entry);
      }

      final backendViewId = entry.backendViewId;
      if (backendViewId != null) {
        final container = _backendContainer(context);
        if (!entry.maximized) {
          entry.restoreRectAfterMaximize = entry.windowRect;
          entry.restoreMonitorIdAfterMaximize = entry.monitorId;
        }
        container
            ?.read(xdgToplevelStatesProvider(backendViewId).notifier)
            .requestMaximize(!entry.maximized);
        return;
      }
      entry.toggleMaximize();
      if (!entry.maximized) {
        entry.windowDock = WindowDock.normal;
      }
    }

    return HeaderBar(
      title: entry.title,
      isActive: isFocused,
      isDraggable: true,
      isMinimizable: true,
      isMaximizable: !entry.maximized,
      isRestorable: entry.maximized,
      isClosable: true,
      onDragStart: (_, p0) => callbacks?.onDragStart(p0),
      onDrag: (_, p0) => callbacks?.onDrag(p0),
      onDragEnd: (_, p0) => callbacks?.onDragEnd(p0),
      onMinimize: (_) {
        final hierarchy =
            context.read<WindowHierarchyState>();
        final manager = WindowManager.of(context);
        final windows = hierarchy.entriesByFocus;
        final backendViewId = entry.backendViewId;
        if (backendViewId != null) {
          final container = _backendContainer(context);
          container
              ?.read(xdgToplevelStatesProvider(backendViewId).notifier)
              .requestVisible(false);
        }
        if (windows.length > 1) {
          final next = windows[windows.length - 2];
          if (manager != null) {
            manager.requestFocus(next);
          } else {
            hierarchy.requestWindowFocus(next);
          }
        }
      },
      onMaximize: (_) => onMaximizeOrRestore(),
      onRestore: (_) => onMaximizeOrRestore(),
      onClose: (_) {
        final manager = WindowManager.of(context);
        final backendViewId = entry.backendViewId;
        if (backendViewId != null) {
          final container = _backendContainer(context);
          container?.read(platformApiProvider.notifier).closeView(backendViewId);
          return;
        }
        if (manager != null) {
          manager.popWindow(entry);
          return;
        }
        context.read<WindowHierarchyState>().popWindowEntry(entry);
      },
    );
  }
}
