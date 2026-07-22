import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:jdwm/src/adapters/riverpod/providers/subsurface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/ui/common/subsurface.dart';
import 'package:jdwm/src/ui/common/surface_size.dart';
import 'package:jdwm/src/ui/common/view_input_listener.dart';

class Surface extends ConsumerWidget {
  final int viewId;

  const Surface({
    super.key,
    required this.viewId,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return SurfaceSize(
      viewId: viewId,
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          _Subsurfaces(
            viewId: viewId,
            layer: _SubsurfaceLayer.below,
          ),
          ViewInputListener(
            viewId: viewId,
            child: Consumer(
              builder: (BuildContext context, WidgetRef ref, Widget? child) {
                final textureKey = ref.watch(
                    surfaceStatesProvider(viewId).select((v) => v.textureKey));
                final textureId = ref.watch(
                    surfaceStatesProvider(viewId).select((v) => v.textureId));
                final surfaceSize = ref.watch(
                    surfaceStatesProvider(viewId).select((v) => v.surfaceSize));
                final bufferSourceBox = ref.watch(surfaceStatesProvider(viewId)
                    .select((v) => v.bufferSourceBox));
                final bufferSize = ref.watch(
                    surfaceStatesProvider(viewId).select((v) => v.bufferSize));

                return _SurfaceTexture(
                  textureKey: textureKey,
                  textureId: textureId,
                  surfaceSize: surfaceSize,
                  bufferSourceBox: bufferSourceBox,
                  bufferSize: bufferSize,
                );
              },
            ),
          ),
          _Subsurfaces(
            viewId: viewId,
            layer: _SubsurfaceLayer.above,
          ),
        ],
      ),
    );
  }
}

class _SurfaceTexture extends StatelessWidget {
  final Key textureKey;
  final int textureId;
  final Size surfaceSize;
  final Rect bufferSourceBox;
  final Size bufferSize;

  const _SurfaceTexture({
    required this.textureKey,
    required this.textureId,
    required this.surfaceSize,
    required this.bufferSourceBox,
    required this.bufferSize,
  });

  @override
  Widget build(BuildContext context) {
    if (surfaceSize.isEmpty ||
        bufferSize.isEmpty ||
        bufferSourceBox.isEmpty ||
        bufferSourceBox.width <= 0 ||
        bufferSourceBox.height <= 0) {
      return SizedBox.fromSize(
        size: surfaceSize,
        child: Texture(
          key: textureKey,
          filterQuality: FilterQuality.medium,
          textureId: textureId,
        ),
      );
    }

    final scaleX = surfaceSize.width / bufferSourceBox.width;
    final scaleY = surfaceSize.height / bufferSourceBox.height;
    final scaledBufferSize = Size(
      bufferSize.width * scaleX,
      bufferSize.height * scaleY,
    );

    return SizedBox.fromSize(
      size: surfaceSize,
      child: ClipRect(
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            Positioned(
              left: -bufferSourceBox.left * scaleX,
              top: -bufferSourceBox.top * scaleY,
              width: scaledBufferSize.width,
              height: scaledBufferSize.height,
              child: Texture(
                key: textureKey,
                filterQuality: FilterQuality.medium,
                textureId: textureId,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _Subsurfaces extends ConsumerWidget {
  final int viewId;
  final _SubsurfaceLayer layer;

  const _Subsurfaces({
    required this.viewId,
    required this.layer,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final selector = layer == _SubsurfaceLayer.below
        ? (SurfaceState ss) => ss.subsurfacesBelow
        : (SurfaceState ss) => ss.subsurfacesAbove;

    List<Widget> subsurfaces = ref
        .watch(surfaceStatesProvider(viewId).select(selector))
        .where((id) =>
            ref.watch(subsurfaceStatesProvider(id).select((ss) => ss.mapped)))
        .map((id) => Subsurface(viewId: id))
        .toList();

    return Stack(
      clipBehavior: Clip.none,
      children: subsurfaces,
    );
  }
}

enum _SubsurfaceLayer { below, above }
