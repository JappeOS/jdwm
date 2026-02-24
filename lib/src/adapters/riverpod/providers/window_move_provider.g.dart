// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_move_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(WindowMove)
final windowMoveProvider = WindowMoveFamily._();

final class WindowMoveProvider
    extends $NotifierProvider<WindowMove, WindowMoveState> {
  WindowMoveProvider._(
      {required WindowMoveFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'windowMoveProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$windowMoveHash();

  @override
  String toString() {
    return r'windowMoveProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  WindowMove create() => WindowMove();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(WindowMoveState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<WindowMoveState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is WindowMoveProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$windowMoveHash() => r'277af0c0b8dbeead9de14c6457e69d7b632f938e';

final class WindowMoveFamily extends $Family
    with
        $ClassFamilyOverride<WindowMove, WindowMoveState, WindowMoveState,
            WindowMoveState, int> {
  WindowMoveFamily._()
      : super(
          retry: null,
          name: r'windowMoveProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  WindowMoveProvider call(
    int viewId,
  ) =>
      WindowMoveProvider._(argument: viewId, from: this);

  @override
  String toString() => r'windowMoveProvider';
}

abstract class _$WindowMove extends $Notifier<WindowMoveState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  WindowMoveState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<WindowMoveState, WindowMoveState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<WindowMoveState, WindowMoveState>,
        WindowMoveState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
