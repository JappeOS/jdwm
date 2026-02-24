// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'surface_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(surfaceWidget)
final surfaceWidgetProvider = SurfaceWidgetFamily._();

final class SurfaceWidgetProvider
    extends $FunctionalProvider<Surface, Surface, Surface>
    with $Provider<Surface> {
  SurfaceWidgetProvider._(
      {required SurfaceWidgetFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'surfaceWidgetProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$surfaceWidgetHash();

  @override
  String toString() {
    return r'surfaceWidgetProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $ProviderElement<Surface> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  Surface create(Ref ref) {
    final argument = this.argument as int;
    return surfaceWidget(
      ref,
      argument,
    );
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(Surface value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<Surface>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is SurfaceWidgetProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$surfaceWidgetHash() => r'c1ff8eac9d2917a36bebe3639369a0405909540c';

final class SurfaceWidgetFamily extends $Family
    with $FunctionalFamilyOverride<Surface, int> {
  SurfaceWidgetFamily._()
      : super(
          retry: null,
          name: r'surfaceWidgetProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  SurfaceWidgetProvider call(
    int viewId,
  ) =>
      SurfaceWidgetProvider._(argument: viewId, from: this);

  @override
  String toString() => r'surfaceWidgetProvider';
}

@ProviderFor(SurfaceStates)
final surfaceStatesProvider = SurfaceStatesFamily._();

final class SurfaceStatesProvider
    extends $NotifierProvider<SurfaceStates, SurfaceState> {
  SurfaceStatesProvider._(
      {required SurfaceStatesFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'surfaceStatesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$surfaceStatesHash();

  @override
  String toString() {
    return r'surfaceStatesProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  SurfaceStates create() => SurfaceStates();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(SurfaceState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<SurfaceState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is SurfaceStatesProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$surfaceStatesHash() => r'6ea65baa9f8d54e164532c96e5a0e866f5866c70';

final class SurfaceStatesFamily extends $Family
    with
        $ClassFamilyOverride<SurfaceStates, SurfaceState, SurfaceState,
            SurfaceState, int> {
  SurfaceStatesFamily._()
      : super(
          retry: null,
          name: r'surfaceStatesProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  SurfaceStatesProvider call(
    int viewId,
  ) =>
      SurfaceStatesProvider._(argument: viewId, from: this);

  @override
  String toString() => r'surfaceStatesProvider';
}

abstract class _$SurfaceStates extends $Notifier<SurfaceState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  SurfaceState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<SurfaceState, SurfaceState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<SurfaceState, SurfaceState>,
        SurfaceState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
