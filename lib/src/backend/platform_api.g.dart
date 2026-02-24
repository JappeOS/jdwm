// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'platform_api.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(MappedWindowList)
final mappedWindowListProvider = MappedWindowListProvider._();

final class MappedWindowListProvider
    extends $NotifierProvider<MappedWindowList, IList<int>> {
  MappedWindowListProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'mappedWindowListProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$mappedWindowListHash();

  @$internal
  @override
  MappedWindowList create() => MappedWindowList();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(IList<int> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<IList<int>>(value),
    );
  }
}

String _$mappedWindowListHash() => r'9e99ef4b404610400dae4d1f79c5eff6674737d2';

abstract class _$MappedWindowList extends $Notifier<IList<int>> {
  IList<int> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<IList<int>, IList<int>>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<IList<int>, IList<int>>, IList<int>, Object?, Object?>;
    element.handleCreate(ref, build);
  }
}

@ProviderFor(WindowMappedStream)
final windowMappedStreamProvider = WindowMappedStreamProvider._();

final class WindowMappedStreamProvider
    extends $StreamNotifierProvider<WindowMappedStream, int> {
  WindowMappedStreamProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'windowMappedStreamProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$windowMappedStreamHash();

  @$internal
  @override
  WindowMappedStream create() => WindowMappedStream();
}

String _$windowMappedStreamHash() =>
    r'406e042fd1cca4b6529cf742bdffe5b4ccdd8c1d';

abstract class _$WindowMappedStream extends $StreamNotifier<int> {
  Stream<int> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<AsyncValue<int>, int>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<AsyncValue<int>, int>, AsyncValue<int>, Object?, Object?>;
    element.handleCreate(ref, build);
  }
}

@ProviderFor(_textInputEventStreamById)
final _textInputEventStreamByIdProvider = _TextInputEventStreamByIdFamily._();

final class _TextInputEventStreamByIdProvider
    extends $FunctionalProvider<AsyncValue<dynamic>, dynamic, Stream<dynamic>>
    with $FutureModifier<dynamic>, $StreamProvider<dynamic> {
  _TextInputEventStreamByIdProvider._(
      {required _TextInputEventStreamByIdFamily super.from,
      required int super.argument})
      : super(
          retry: null,
          name: r'_textInputEventStreamByIdProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$_textInputEventStreamByIdHash();

  @override
  String toString() {
    return r'_textInputEventStreamByIdProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $StreamProviderElement<dynamic> $createElement($ProviderPointer pointer) =>
      $StreamProviderElement(pointer);

  @override
  Stream<dynamic> create(Ref ref) {
    final argument = this.argument as int;
    return _textInputEventStreamById(
      ref,
      argument,
    );
  }

  @override
  bool operator ==(Object other) {
    return other is _TextInputEventStreamByIdProvider &&
        other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$_textInputEventStreamByIdHash() =>
    r'a73a40883174b54068c1f2c750106f78ce98e647';

final class _TextInputEventStreamByIdFamily extends $Family
    with $FunctionalFamilyOverride<Stream<dynamic>, int> {
  _TextInputEventStreamByIdFamily._()
      : super(
          retry: null,
          name: r'_textInputEventStreamByIdProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  _TextInputEventStreamByIdProvider call(
    int viewId,
  ) =>
      _TextInputEventStreamByIdProvider._(argument: viewId, from: this);

  @override
  String toString() => r'_textInputEventStreamByIdProvider';
}

@ProviderFor(textInputEventStream)
final textInputEventStreamProvider = TextInputEventStreamFamily._();

final class TextInputEventStreamProvider extends $FunctionalProvider<
        AsyncValue<TextInputEventType>,
        TextInputEventType,
        FutureOr<TextInputEventType>>
    with
        $FutureModifier<TextInputEventType>,
        $FutureProvider<TextInputEventType> {
  TextInputEventStreamProvider._(
      {required TextInputEventStreamFamily super.from,
      required int super.argument})
      : super(
          retry: null,
          name: r'textInputEventStreamProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$textInputEventStreamHash();

  @override
  String toString() {
    return r'textInputEventStreamProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $FutureProviderElement<TextInputEventType> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<TextInputEventType> create(Ref ref) {
    final argument = this.argument as int;
    return textInputEventStream(
      ref,
      argument,
    );
  }

  @override
  bool operator ==(Object other) {
    return other is TextInputEventStreamProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$textInputEventStreamHash() =>
    r'70dfe98a2dd76ba7c8754724471c049201bc60d9';

final class TextInputEventStreamFamily extends $Family
    with $FunctionalFamilyOverride<FutureOr<TextInputEventType>, int> {
  TextInputEventStreamFamily._()
      : super(
          retry: null,
          name: r'textInputEventStreamProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  TextInputEventStreamProvider call(
    int viewId,
  ) =>
      TextInputEventStreamProvider._(argument: viewId, from: this);

  @override
  String toString() => r'textInputEventStreamProvider';
}

@ProviderFor(WindowUnmappedStream)
final windowUnmappedStreamProvider = WindowUnmappedStreamProvider._();

final class WindowUnmappedStreamProvider
    extends $StreamNotifierProvider<WindowUnmappedStream, int> {
  WindowUnmappedStreamProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'windowUnmappedStreamProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$windowUnmappedStreamHash();

  @$internal
  @override
  WindowUnmappedStream create() => WindowUnmappedStream();
}

String _$windowUnmappedStreamHash() =>
    r'27c0e325f702f372c2022ac085b09374437e50c1';

abstract class _$WindowUnmappedStream extends $StreamNotifier<int> {
  Stream<int> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<AsyncValue<int>, int>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<AsyncValue<int>, int>, AsyncValue<int>, Object?, Object?>;
    element.handleCreate(ref, build);
  }
}

@ProviderFor(PlatformApi)
final platformApiProvider = PlatformApiProvider._();

final class PlatformApiProvider
    extends $NotifierProvider<PlatformApi, PlatformApiState> {
  PlatformApiProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'platformApiProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$platformApiHash();

  @$internal
  @override
  PlatformApi create() => PlatformApi();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(PlatformApiState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<PlatformApiState>(value),
    );
  }
}

String _$platformApiHash() => r'92befcf5de5f308a6e51bc24eed063ea2cda25a1';

abstract class _$PlatformApi extends $Notifier<PlatformApiState> {
  PlatformApiState build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<PlatformApiState, PlatformApiState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<PlatformApiState, PlatformApiState>,
        PlatformApiState,
        Object?,
        Object?>;
    element.handleCreate(ref, build);
  }
}
