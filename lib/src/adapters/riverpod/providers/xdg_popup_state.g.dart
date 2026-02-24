// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'xdg_popup_state.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(popupWidget)
final popupWidgetProvider = PopupWidgetFamily._();

final class PopupWidgetProvider extends $FunctionalProvider<Popup, Popup, Popup>
    with $Provider<Popup> {
  PopupWidgetProvider._(
      {required PopupWidgetFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'popupWidgetProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$popupWidgetHash();

  @override
  String toString() {
    return r'popupWidgetProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $ProviderElement<Popup> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  Popup create(Ref ref) {
    final argument = this.argument as int;
    return popupWidget(
      ref,
      argument,
    );
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(Popup value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<Popup>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is PopupWidgetProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$popupWidgetHash() => r'e8707948656bd1c97f5eee7bc0a558f9c5f67e3f';

final class PopupWidgetFamily extends $Family
    with $FunctionalFamilyOverride<Popup, int> {
  PopupWidgetFamily._()
      : super(
          retry: null,
          name: r'popupWidgetProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  PopupWidgetProvider call(
    int viewId,
  ) =>
      PopupWidgetProvider._(argument: viewId, from: this);

  @override
  String toString() => r'popupWidgetProvider';
}

@ProviderFor(XdgPopupStates)
final xdgPopupStatesProvider = XdgPopupStatesFamily._();

final class XdgPopupStatesProvider
    extends $NotifierProvider<XdgPopupStates, XdgPopupState> {
  XdgPopupStatesProvider._(
      {required XdgPopupStatesFamily super.from, required int super.argument})
      : super(
          retry: null,
          name: r'xdgPopupStatesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$xdgPopupStatesHash();

  @override
  String toString() {
    return r'xdgPopupStatesProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  XdgPopupStates create() => XdgPopupStates();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(XdgPopupState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<XdgPopupState>(value),
    );
  }

  @override
  bool operator ==(Object other) {
    return other is XdgPopupStatesProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$xdgPopupStatesHash() => r'90943e3eabb5904590d472efa5ef6e3c6a7e5215';

final class XdgPopupStatesFamily extends $Family
    with
        $ClassFamilyOverride<XdgPopupStates, XdgPopupState, XdgPopupState,
            XdgPopupState, int> {
  XdgPopupStatesFamily._()
      : super(
          retry: null,
          name: r'xdgPopupStatesProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  XdgPopupStatesProvider call(
    int viewId,
  ) =>
      XdgPopupStatesProvider._(argument: viewId, from: this);

  @override
  String toString() => r'xdgPopupStatesProvider';
}

abstract class _$XdgPopupStates extends $Notifier<XdgPopupState> {
  late final _$args = ref.$arg as int;
  int get viewId => _$args;

  XdgPopupState build(
    int viewId,
  );
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<XdgPopupState, XdgPopupState>;
    final element = ref.element as $ClassProviderElement<
        AnyNotifier<XdgPopupState, XdgPopupState>,
        XdgPopupState,
        Object?,
        Object?>;
    element.handleCreate(
        ref,
        () => build(
              _$args,
            ));
  }
}
