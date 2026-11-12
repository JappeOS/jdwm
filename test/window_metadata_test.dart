import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:jdwm/src/window_metadata.dart';

void main() {
  test(
      'metadata owns pixels, rejects invalid variants and resets with a new revision',
      () {
    final bytes = Uint8List.fromList([128, 0, 0, 128]);
    final metadata = ClientWindowMetadata.fromMessage({
      'revision': 1,
      'wm_class': 'Wine',
      'images': [
        {'width': 1, 'height': 1, 'scale': 1, 'rgba': bytes},
        {'width': 1025, 'height': 1, 'scale': 1, 'rgba': bytes},
        {'width': 1, 'height': 1, 'scale': 0, 'rgba': bytes},
        {'width': 2, 'height': 1, 'scale': 1, 'rgba': bytes},
      ]
    })!;
    bytes[0] = 0;
    expect(metadata.images.single.rgba[0], 128);
    expect(() => metadata.images.single.rgba[0] = 0, throwsUnsupportedError);
    final container = ProviderContainer();
    addTearDown(container.dispose);
    final store = container.read(windowMetadataProvider.notifier);
    store.update(1, metadata);
    store.update(1, const ClientWindowMetadata(revision: 1));
    expect(container.read(windowMetadataProvider)[1]!.wmClass, 'Wine');
    store.update(1, const ClientWindowMetadata(revision: 2));
    expect(container.read(windowMetadataProvider)[1]!.images, isEmpty);
    store.remove(1);
    expect(container.read(windowMetadataProvider), isEmpty);
  });
}
