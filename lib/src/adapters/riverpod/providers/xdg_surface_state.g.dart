// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'xdg_surface_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(XdgSurfaceStates)
final xdgSurfaceStatesProvider = XdgSurfaceStatesFamily._();

final class XdgSurfaceStatesProvider
    extends $NotifierProvider<XdgSurfaceStates, XdgSurfaceState> {
  XdgSurfaceStatesProvider._(
      {required XdgSurfaceStatesFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'xdgSurfaceStatesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$xdgSurfaceStatesHash();

  @override
  String toString() {
    return r'xdgSurfaceStatesProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  XdgSurfaceStates create() => XdgSurfaceStates();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(XdgSurfaceState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<XdgSurfaceState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is XdgSurfaceStatesProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$xdgSurfaceStatesHash() => r'760fabafe3d0a251621587fc91050d43bd7da42a';

final class XdgSurfaceStatesFamily extends $Family
    with
        $ClassFamilyOverride<XdgSurfaceStates, XdgSurfaceState, XdgSurfaceState,
            XdgSurfaceState, int> {
  XdgSurfaceStatesFamily._()
      : super(
          retry: null,
          name: r'xdgSurfaceStatesProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  XdgSurfaceStatesProvider call(
    int viewId,
  ) =>
      XdgSurfaceStatesProvider._(argument: viewId, from: this);

  @override
  String toString() => r'xdgSurfaceStatesProvider';
}

abstract class _$XdgSurfaceStates extends $Notifier<XdgSurfaceState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  XdgSurfaceState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<XdgSurfaceState, XdgSurfaceState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<XdgSurfaceState, XdgSurfaceState>,
        XdgSurfaceState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
