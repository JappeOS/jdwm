import 'package:flutter/widgets.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../backend/platform_api.dart';
import '../../adapters/riverpod/providers/managed_surface_rect.dart';
import '../../util/rect_overflow_box.dart';
import 'surface.dart';

/// A live, input-transparent preview of a mapped client's window content.
///
/// Use the inventory entry's [viewId] inside the window manager's backend
/// provider scope (for example, in a monitor overlay). Supply [width] and
/// [height] or size the preview with parent constraints. By default the whole
/// window fits inside the preview with its aspect ratio preserved.
///
/// The preview includes mapped subsurfaces. A minimized client can display its
/// last submitted frame. An unmapped window leaves the preview area empty.
class WindowPreview extends ConsumerWidget {
  final int viewId;
  final double? width;
  final double? height;
  final BoxFit fit;

  const WindowPreview({
    super.key,
    required this.viewId,
    this.width,
    this.height,
    this.fit = BoxFit.contain,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final mapped = ref.watch(
      mappedWindowListProvider.select((windows) => windows.contains(viewId)),
    );

    Widget? content;
    
    if (mapped) {
      final viewport = ref.watch(managedSurfaceRectProvider(viewId));
      if (viewport.isFinite && !viewport.isEmpty) {
        content = FittedBox(
          fit: fit,
          clipBehavior: Clip.hardEdge,
          child: ClipRect(
            child: RectOverflowBox(
              rect: viewport,
              child: Surface.preview(key: ValueKey(viewId), viewId: viewId),
            ),
          ),
        );
      }
    }
    
    return IgnorePointer(
      child: SizedBox(width: width, height: height, child: content),
    );
  }
}
