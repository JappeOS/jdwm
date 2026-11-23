import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:jdwm/src/backend/platform_api.dart';
import 'package:jdwm/src/adapters/riverpod/providers/subsurface_state.dart';
import 'package:jdwm/src/adapters/riverpod/providers/surface_state.dart';
import 'package:jdwm/src/ui/common/subsurface.dart';
import 'package:jdwm/src/ui/common/surface_size.dart';
import 'package:jdwm/src/ui/common/view_input_listener.dart';

class Surface extends ConsumerStatefulWidget {
  final int viewId;
  final bool _preview;

  const Surface({
    super.key,
    required this.viewId,
  }) : _preview = false;

  const Surface.preview({
    super.key,
    required this.viewId,
  }) : _preview = true;

  @override
  ConsumerState<Surface> createState() => _SurfaceState();
}

class _SurfaceState extends ConsumerState<Surface> {
  late final textures = ref.read(platformApiProvider.notifier).surfaceTextures;

  @override
  void initState() {
    super.initState();
    textures.attach(widget.viewId);
  }

  @override
  void didUpdateWidget(Surface oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.viewId != widget.viewId) {
      textures.detach(oldWidget.viewId);
      textures.attach(widget.viewId);
    }
  }

  @override
  void dispose() {
    textures.detach(widget.viewId);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final viewId = widget.viewId;
    final texture = _SurfaceTexture(viewId: viewId);
    final surface = SurfaceSize(
      viewId: viewId,
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          _Subsurfaces(
            viewId: viewId,
            preview: widget._preview,
            layer: _SubsurfaceLayer.below,
          ),
          if (widget._preview)
            texture
          else
            ViewInputListener(viewId: viewId, child: texture),
          _Subsurfaces(
            viewId: viewId,
            preview: widget._preview,
            layer: _SubsurfaceLayer.above,
          ),
        ],
      ),
    );
    if (widget._preview) return surface;
    return CompositedTransformTarget(
        link: ref
            .watch(surfaceStatesProvider(viewId).select((s) => s.popupAnchor)),
        child: surface);
  }
}

class _SurfaceTexture extends ConsumerWidget {
  final int viewId;
  const _SurfaceTexture({required this.viewId});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final provider = surfaceStatesProvider(viewId);
    final textureId = ref.watch(provider.select((s) => s.textureId));
    final surfaceSize = ref.watch(provider.select((s) => s.surfaceSize));
    final bufferSourceBox =
        ref.watch(provider.select((s) => s.bufferSourceBox));
    final bufferSize = ref.watch(provider.select((s) => s.bufferSize));
    if (surfaceSize.isEmpty ||
        bufferSize.isEmpty ||
        bufferSourceBox.isEmpty ||
        bufferSourceBox.width <= 0 ||
        bufferSourceBox.height <= 0) {
      return SizedBox.fromSize(
        size: surfaceSize,
        child: Texture(
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
  final bool preview;

  const _Subsurfaces({
    required this.viewId,
    required this.layer,
    required this.preview,
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
        .map((id) => Subsurface(
              key: ValueKey(id),
              viewId: id,
              child: preview ? Surface.preview(viewId: id) : null,
            ))
        .toList();

    return Stack(
      clipBehavior: Clip.none,
      children: subsurfaces,
    );
  }
}

enum _SubsurfaceLayer { below, above }
