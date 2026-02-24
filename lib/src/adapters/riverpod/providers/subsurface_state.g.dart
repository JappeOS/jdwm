// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'subsurface_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(subsurfaceWidget)
final subsurfaceWidgetProvider = SubsurfaceWidgetFamily._();

final class SubsurfaceWidgetProvider
    extends $FunctionalProvider<Subsurface, Subsurface, Subsurface>
    with $Provider<Subsurface> {
  SubsurfaceWidgetProvider._(
      {required SubsurfaceWidgetFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'subsurfaceWidgetProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$subsurfaceWidgetHash();

  @override
  String toString() {
    return r'subsurfaceWidgetProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $ProviderElement<Subsurface> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  Subsurface create(Ref ref) {
    final argument = this.argument as int;
    return subsurfaceWidget(
      ref,
      argument,
    );
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(Subsurface value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<Subsurface>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is SubsurfaceWidgetProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$subsurfaceWidgetHash() => r'a1c0047a157e74259ca9f6123be4a3bb862b8fe3';

final class SubsurfaceWidgetFamily extends $Family
    with $FunctionalFamilyOverride<Subsurface, int> {
  SubsurfaceWidgetFamily._()
      : super(
          retry: null,
          name: r'subsurfaceWidgetProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  SubsurfaceWidgetProvider call(
    int viewId,
  ) =>
      SubsurfaceWidgetProvider._(argument: viewId, from: this);

  @override
  String toString() => r'subsurfaceWidgetProvider';
}

@ProviderFor(SubsurfaceStates)
final subsurfaceStatesProvider = SubsurfaceStatesFamily._();

final class SubsurfaceStatesProvider
    extends $NotifierProvider<SubsurfaceStates, SubsurfaceState> {
  SubsurfaceStatesProvider._(
      {required SubsurfaceStatesFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'subsurfaceStatesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$subsurfaceStatesHash();

  @override
  String toString() {
    return r'subsurfaceStatesProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  SubsurfaceStates create() => SubsurfaceStates();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(SubsurfaceState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<SubsurfaceState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is SubsurfaceStatesProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$subsurfaceStatesHash() => r'60f3a340b9df78579fe67d00f1ab44cb42866557';

final class SubsurfaceStatesFamily extends $Family
    with
        $ClassFamilyOverride<SubsurfaceStates, SubsurfaceState, SubsurfaceState,
            SubsurfaceState, int> {
  SubsurfaceStatesFamily._()
      : super(
          retry: null,
          name: r'subsurfaceStatesProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  SubsurfaceStatesProvider call(
    int viewId,
  ) =>
      SubsurfaceStatesProvider._(argument: viewId, from: this);

  @override
  String toString() => r'subsurfaceStatesProvider';
}

abstract class _$SubsurfaceStates extends $Notifier<SubsurfaceState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  SubsurfaceState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<SubsurfaceState, SubsurfaceState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<SubsurfaceState, SubsurfaceState>,
        SubsurfaceState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
