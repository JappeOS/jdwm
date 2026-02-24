// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_resize_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(WindowResize)
final windowResizeProvider = WindowResizeFamily._();

final class WindowResizeProvider
    extends $NotifierProvider<WindowResize, ResizerState> {
  WindowResizeProvider._(
      {required WindowResizeFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'windowResizeProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$windowResizeHash();

  @override
  String toString() {
    return r'windowResizeProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  WindowResize create() => WindowResize();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(ResizerState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<ResizerState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is WindowResizeProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$windowResizeHash() => r'65bdbbc30edc9ae6c4d7011face971607e7d9b5d';

final class WindowResizeFamily extends $Family
    with
        $ClassFamilyOverride<WindowResize, ResizerState, ResizerState,
            ResizerState, int> {
  WindowResizeFamily._()
      : super(
          retry: null,
          name: r'windowResizeProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  WindowResizeProvider call(
    int viewId,
  ) =>
      WindowResizeProvider._(argument: viewId, from: this);

  @override
  String toString() => r'windowResizeProvider';
}

abstract class _$WindowResize extends $Notifier<ResizerState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  ResizerState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<ResizerState, ResizerState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<ResizerState, ResizerState>,
        ResizerState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
