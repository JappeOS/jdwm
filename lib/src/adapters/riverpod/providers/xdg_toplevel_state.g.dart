// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'xdg_toplevel_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(xdgToplevelSurfaceWidget)
final xdgToplevelSurfaceWidgetProvider = XdgToplevelSurfaceWidgetFamily._();

final class XdgToplevelSurfaceWidgetProvider extends $FunctionalProvider<
    XdgToplevelSurface,
    XdgToplevelSurface,
    XdgToplevelSurface> with $Provider<XdgToplevelSurface> {
  XdgToplevelSurfaceWidgetProvider._(
      {required XdgToplevelSurfaceWidgetFamily super.from,
      required int super.argument})
      : super(
          retry: null,
          name: r'xdgToplevelSurfaceWidgetProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$xdgToplevelSurfaceWidgetHash();

  @override
  String toString() {
    return r'xdgToplevelSurfaceWidgetProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $ProviderElement<XdgToplevelSurface> $createElement(
          $ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  XdgToplevelSurface create(Ref ref) {
    final argument = this.argument as int;
    return xdgToplevelSurfaceWidget(
      ref,
      argument,
    );
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(XdgToplevelSurface value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<XdgToplevelSurface>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is XdgToplevelSurfaceWidgetProvider &&
        other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$xdgToplevelSurfaceWidgetHash() =>
    r'c452693991e5b07ff5cd49e46a5c8cfab56a0da5';

final class XdgToplevelSurfaceWidgetFamily extends $Family
    with $FunctionalFamilyOverride<XdgToplevelSurface, int> {
  XdgToplevelSurfaceWidgetFamily._()
      : super(
          retry: null,
          name: r'xdgToplevelSurfaceWidgetProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  XdgToplevelSurfaceWidgetProvider call(
    int viewId,
  ) =>
      XdgToplevelSurfaceWidgetProvider._(argument: viewId, from: this);

  @override
  String toString() => r'xdgToplevelSurfaceWidgetProvider';
}

@ProviderFor(XdgToplevelStates)
final xdgToplevelStatesProvider = XdgToplevelStatesFamily._();

final class XdgToplevelStatesProvider
    extends $NotifierProvider<XdgToplevelStates, XdgToplevelState> {
  XdgToplevelStatesProvider._(
      {required XdgToplevelStatesFamily super.from,
      required int super.argument})
      : super(
          retry: null,
          name: r'xdgToplevelStatesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$xdgToplevelStatesHash();

  @override
  String toString() {
    return r'xdgToplevelStatesProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  XdgToplevelStates create() => XdgToplevelStates();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(XdgToplevelState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<XdgToplevelState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is XdgToplevelStatesProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$xdgToplevelStatesHash() => r'41bd3b0f400421da45824e346b3bf35a98012753';

final class XdgToplevelStatesFamily extends $Family
    with
        $ClassFamilyOverride<XdgToplevelStates, XdgToplevelState,
            XdgToplevelState, XdgToplevelState, int> {
  XdgToplevelStatesFamily._()
      : super(
          retry: null,
          name: r'xdgToplevelStatesProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  XdgToplevelStatesProvider call(
    int viewId,
  ) =>
      XdgToplevelStatesProvider._(argument: viewId, from: this);

  @override
  String toString() => r'xdgToplevelStatesProvider';
}

abstract class _$XdgToplevelStates extends $Notifier<XdgToplevelState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  XdgToplevelState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<XdgToplevelState, XdgToplevelState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<XdgToplevelState, XdgToplevelState>,
        XdgToplevelState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
