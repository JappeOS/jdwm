// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_stack_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(WindowStack)
final windowStackProvider = WindowStackProvider._();

final class WindowStackProvider
    extends $NotifierProvider<WindowStack, WindowStackState> {
  WindowStackProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'windowStackProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$windowStackHash();

  @$internal
  @override
  WindowStack create() => WindowStack();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(WindowStackState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<WindowStackState>(value),
    );
  }
}

String _$windowStackHash() => r'8910d03c85cda3d4f36646c3a292fd9d84a6b482';

abstract class _$WindowStack extends $Notifier<WindowStackState> {
  WindowStackState build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<WindowStackState, WindowStackState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<WindowStackState, WindowStackState>,
        WindowStackState,
        Object?,
        Object?>;
    element.handleCreate(ref, build);
  }
}
