// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'virtual_keyboard_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(VirtualKeyboardStateNotifier)
final virtualKeyboardStateProvider = VirtualKeyboardStateNotifierFamily._();

final class VirtualKeyboardStateNotifierProvider extends $NotifierProvider<
    VirtualKeyboardStateNotifier, VirtualKeyboardState> {
  VirtualKeyboardStateNotifierProvider._(
      {required VirtualKeyboardStateNotifierFamily super.from,
      required int super.argument})
      : super(
          retry: null,
          name: r'virtualKeyboardStateProvider',
          isAutoDispose: true,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$virtualKeyboardStateNotifierHash();

  @override
  String toString() {
    return r'virtualKeyboardStateProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  VirtualKeyboardStateNotifier create() => VirtualKeyboardStateNotifier();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(VirtualKeyboardState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<VirtualKeyboardState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is VirtualKeyboardStateNotifierProvider &&
        other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$virtualKeyboardStateNotifierHash() =>
    r'509c2003b6d9df7442185d20145faceeedc05022';

final class VirtualKeyboardStateNotifierFamily extends $Family
    with
        $ClassFamilyOverride<VirtualKeyboardStateNotifier, VirtualKeyboardState,
            VirtualKeyboardState, VirtualKeyboardState, int> {
  VirtualKeyboardStateNotifierFamily._()
      : super(
          retry: null,
          name: r'virtualKeyboardStateProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: true,
        );

  VirtualKeyboardStateNotifierProvider call(
    int viewId,
  ) =>
      VirtualKeyboardStateNotifierProvider._(argument: viewId, from: this);

  @override
  String toString() => r'virtualKeyboardStateProvider';
}

abstract class _$VirtualKeyboardStateNotifier
    extends $Notifier<VirtualKeyboardState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  VirtualKeyboardState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<VirtualKeyboardState, VirtualKeyboardState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<VirtualKeyboardState, VirtualKeyboardState>,
        VirtualKeyboardState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
