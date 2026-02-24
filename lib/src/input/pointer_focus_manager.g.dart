// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'pointer_focus_manager.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(pointerFocusManager)
final pointerFocusManagerProvider = PointerFocusManagerProvider._();

final class PointerFocusManagerProvider extends $FunctionalProvider<
    PointerFocusManager,
    PointerFocusManager,
    PointerFocusManager> with $Provider<PointerFocusManager> {
  PointerFocusManagerProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'pointerFocusManagerProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$pointerFocusManagerHash();

  @$internal
  @override
  $ProviderElement<PointerFocusManager> $createElement(
          $ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  PointerFocusManager create(Ref ref) {
    return pointerFocusManager(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(PointerFocusManager value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<PointerFocusManager>(value),
    );
  }
}

String _$pointerFocusManagerHash() =>
    r'583484e12fb0ed8d2e94063d261bcb58f7096759';
