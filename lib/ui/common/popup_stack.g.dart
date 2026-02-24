// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'popup_stack.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(popupStackGlobalKey)
final popupStackGlobalKeyProvider = PopupStackGlobalKeyProvider._();

final class PopupStackGlobalKeyProvider extends $FunctionalProvider<
        GlobalKey<State<StatefulWidget>>,
        GlobalKey<State<StatefulWidget>>,
        GlobalKey<State<StatefulWidget>>>
    with $Provider<GlobalKey<State<StatefulWidget>>> {
  PopupStackGlobalKeyProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'popupStackGlobalKeyProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$popupStackGlobalKeyHash();

  @$internal
  @override
  $ProviderElement<GlobalKey<State<StatefulWidget>>> $createElement(
          $ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  GlobalKey<State<StatefulWidget>> create(Ref ref) {
    return popupStackGlobalKey(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(GlobalKey<State<StatefulWidget>> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride:
          $SyncValueProvider<GlobalKey<State<StatefulWidget>>>(value),
    );
  }
}

String _$popupStackGlobalKeyHash() =>
    r'351fdccaa4b4ce24c8568cc5df75c2b3575cbbb0';

@ProviderFor(PopupStackChildren)
final popupStackChildrenProvider = PopupStackChildrenProvider._();

final class PopupStackChildrenProvider
    extends $NotifierProvider<PopupStackChildren, IList<int>> {
  PopupStackChildrenProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'popupStackChildrenProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$popupStackChildrenHash();

  @$internal
  @override
  PopupStackChildren create() => PopupStackChildren();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(IList<int> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<IList<int>>(value),
    );
  }
}

String _$popupStackChildrenHash() =>
    r'03c8602175bf1c73be917deeccf3a38071b935a0';

abstract class _$PopupStackChildren extends $Notifier<IList<int>> {
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
