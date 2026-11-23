import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter/scheduler.dart' show TickerCanceled;
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_popup_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/xdg_surface_state.dart';

class Popup extends StatelessWidget {
  final int viewId;

  const Popup({
    required Key key,
    required this.viewId,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return _Positioner(
      viewId: viewId,
      child: Consumer(
        builder: (BuildContext context, WidgetRef ref, Widget? child) {
          final hasSize = ref.watch(surfaceStatesProvider(viewId).select(
            (s) => s.surfaceSize.isFinite && !s.surfaceSize.isEmpty,
          ));

          if (!hasSize) return const SizedBox.shrink();

          Key key = ref.watch(
              xdgPopupStatesProvider(viewId).select((v) => v.animationsKey));
          return _Animations(
            viewId: viewId,
            key: key,
            child: child!,
          );
        },
        child: Consumer(
          builder: (_, WidgetRef ref, __) {
            return ref.watch(surfaceWidgetProvider(viewId));
          },
        ),
      ),
    );
  }
}

class _Positioner extends ConsumerWidget {
  final int viewId;
  final Widget child;

  const _Positioner({
    Key? key,
    required this.viewId,
    required this.child,
  }) : super(key: key);

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final popup = ref.watch(xdgPopupStatesProvider(viewId));
    final parent = popup.parentViewId;
    final link =
        ref.watch(surfaceStatesProvider(parent).select((s) => s.popupAnchor));
    final parentOrigin = ref.watch(xdgSurfaceStatesProvider(parent)
        .select((s) => s.visibleBounds.topLeft));
    final origin = ref.watch(xdgSurfaceStatesProvider(viewId)
        .select((s) => s.visibleBounds.topLeft));
    return CompositedTransformFollower(
      link: link,
      showWhenUnlinked: false,
      offset: popup.position + parentOrigin - origin,
      child: IgnorePointer(ignoring: popup.isClosing, child: child),
    );
  }
}

class _Animations extends ConsumerStatefulWidget {
  final Widget child;
  final int viewId;

  const _Animations({super.key, required this.viewId, required this.child});

  @override
  ConsumerState<_Animations> createState() => AnimationsState();
}

class AnimationsState extends ConsumerState<_Animations>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller = AnimationController(
    duration: const Duration(milliseconds: 200),
    reverseDuration: const Duration(milliseconds: 100),
    vsync: this,
  )..forward();

  late final Animation<double> _progress =
      _controller.drive(CurveTween(curve: Curves.easeOutCubic));

  @override
  void initState() {
    super.initState();
    ref.listenManual(
        xdgPopupStatesProvider(widget.viewId).select((s) => s.isClosing),
        (_, closing) async {
      if (closing) {
        final owner = ref.read(xdgPopupStatesProvider(widget.viewId).notifier);
        final finish = owner.closeCompletion();
        
        try {
          await _controller.reverse().orCancel;
        } on TickerCanceled {
          // Remapping or disposal cancels a pending close.
        }

        finish();
      } else {
        _controller.forward();
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return FadeTransition(
      opacity: _progress,
      child: AnimatedBuilder(
        animation: _progress,
        child: widget.child,
        builder: (context, child) => Transform.translate(
          offset: Offset(0, -10 * (1 - _progress.value)),
          transformHitTests: false,
          child: child,
        ),
      ),
    );
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }
}
