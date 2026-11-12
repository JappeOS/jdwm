import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

/// Owned, premultiplied RGBA pixels. No filesystem or application policy.
@immutable
class ClientIconImage {
  final int width;
  final int height;
  final int scale;
  final Uint8List rgba;

  ClientIconImage({
    required this.width,
    required this.height,
    required this.scale,
    required Uint8List rgba
  }) : rgba = Uint8List.fromList(rgba).asUnmodifiableView();
}

@immutable
class ClientWindowMetadata {
  final int revision;
  final String wmClass;
  final String wmInstance;
  final String iconName;
  final List<ClientIconImage> images;

  const ClientWindowMetadata({
    this.revision = 0,
    this.wmClass = '',
    this.wmInstance = '',
    this.iconName = '',
    this.images = const [],
  });

  static ClientWindowMetadata? fromMessage(Map event) {
    final revision = event['revision'];
    if (revision is! int || revision <= 0) return null;

    final images = <ClientIconImage>[];
    var bytes = 0;
    final raw = event['images'];
    if (raw is List) {
      for (final item in raw.take(16)) {
        if (item is! Map) continue;

        final w = item['width'], h = item['height'], scale = item['scale'];
        final rgba = item['rgba'];
        if (w is! int ||
            h is! int ||
            scale is! int ||
            rgba is! Uint8List ||
            w <= 0 ||
            h <= 0 ||
            w > 1024 ||
            h > 1024 ||
            scale <= 0 ||
            rgba.length != w * h * 4 ||
            bytes + rgba.length > 8 * 1024 * 1024) {
          continue;
        }

        images.add(
            ClientIconImage(width: w, height: h, scale: scale, rgba: rgba));
        bytes += rgba.length;
      }
    }

    String text(String key) => event[key] is String ? event[key] as String : '';
    return ClientWindowMetadata(
      revision: revision,
      wmClass: text('wm_class'),
      wmInstance: text('wm_instance'),
      iconName: text('icon_name'),
      images: List.unmodifiable(images),
    );
  }
}

final windowMetadataProvider =
    NotifierProvider<WindowMetadataStore, Map<int, ClientWindowMetadata>>(
        WindowMetadataStore.new);

class WindowMetadataStore extends Notifier<Map<int, ClientWindowMetadata>> {
  @override
  Map<int, ClientWindowMetadata> build() => const {};
  void update(int id, ClientWindowMetadata metadata) {
    if ((state[id]?.revision ?? 0) >= metadata.revision) return;
    state = Map.unmodifiable({...state, id: metadata});
  }

  void remove(int id) {
    if (!state.containsKey(id)) return;
    state = Map.unmodifiable({...state}..remove(id));
  }
}
