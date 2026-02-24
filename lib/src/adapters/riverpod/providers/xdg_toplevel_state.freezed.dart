// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'xdg_toplevel_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$XdgToplevelState {
  bool get visible;
  bool get maximized;
  Key get virtualKeyboardKey;
  FocusNode get focusNode;
  Object get interactiveMoveRequested;
  ResizeEdgeObject get interactiveResizeRequested;
  ToplevelDecoration get decoration;
  String get title;
  String get appId;

  /// Create a copy of XdgToplevelState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $XdgToplevelStateCopyWith<XdgToplevelState> get copyWith =>
      _$XdgToplevelStateCopyWithImpl<XdgToplevelState>(
          this as XdgToplevelState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is XdgToplevelState &&
            (identical(other.visible, visible) || other.visible == visible) &&
            (identical(other.maximized, maximized) ||
                other.maximized == maximized) &&
            (identical(other.virtualKeyboardKey, virtualKeyboardKey) ||
                other.virtualKeyboardKey == virtualKeyboardKey) &&
            (identical(other.focusNode, focusNode) ||
                other.focusNode == focusNode) &&
            const DeepCollectionEquality().equals(
                other.interactiveMoveRequested, interactiveMoveRequested) &&
            (identical(other.interactiveResizeRequested,
                    interactiveResizeRequested) ||
                other.interactiveResizeRequested ==
                    interactiveResizeRequested) &&
            (identical(other.decoration, decoration) ||
                other.decoration == decoration) &&
            (identical(other.title, title) || other.title == title) &&
            (identical(other.appId, appId) || other.appId == appId));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType,
      visible,
      maximized,
      virtualKeyboardKey,
      focusNode,
      const DeepCollectionEquality().hash(interactiveMoveRequested),
      interactiveResizeRequested,
      decoration,
      title,
      appId);

  @override
  String toString() {
    return 'XdgToplevelState(visible: $visible, maximized: $maximized, virtualKeyboardKey: $virtualKeyboardKey, focusNode: $focusNode, interactiveMoveRequested: $interactiveMoveRequested, interactiveResizeRequested: $interactiveResizeRequested, decoration: $decoration, title: $title, appId: $appId)';
  }
}

/// @nodoc
abstract mixin class $XdgToplevelStateCopyWith<$Res> {
  factory $XdgToplevelStateCopyWith(
          XdgToplevelState value, $Res Function(XdgToplevelState) _then) =
      _$XdgToplevelStateCopyWithImpl;
  @useResult
  $Res call(
      {bool visible,
      bool maximized,
      Key virtualKeyboardKey,
      FocusNode focusNode,
      Object interactiveMoveRequested,
      ResizeEdgeObject interactiveResizeRequested,
      ToplevelDecoration decoration,
      String title,
      String appId});
}

/// @nodoc
class _$XdgToplevelStateCopyWithImpl<$Res>
    implements $XdgToplevelStateCopyWith<$Res> {
  _$XdgToplevelStateCopyWithImpl(this._self, this._then);

  final XdgToplevelState _self;
  final $Res Function(XdgToplevelState) _then;

  /// Create a copy of XdgToplevelState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? visible = null,
    Object? maximized = null,
    Object? virtualKeyboardKey = null,
    Object? focusNode = null,
    Object? interactiveMoveRequested = null,
    Object? interactiveResizeRequested = null,
    Object? decoration = null,
    Object? title = null,
    Object? appId = null,
  }) {
    return _then(_self.copyWith(
      visible: null == visible
          ? _self.visible
          : visible // ignore: cast_nullable_to_non_nullable
              as bool,
      maximized: null == maximized
          ? _self.maximized
          : maximized // ignore: cast_nullable_to_non_nullable
              as bool,
      virtualKeyboardKey: null == virtualKeyboardKey
          ? _self.virtualKeyboardKey
          : virtualKeyboardKey // ignore: cast_nullable_to_non_nullable
              as Key,
      focusNode: null == focusNode
          ? _self.focusNode
          : focusNode // ignore: cast_nullable_to_non_nullable
              as FocusNode,
      interactiveMoveRequested: null == interactiveMoveRequested
          ? _self.interactiveMoveRequested
          : interactiveMoveRequested,
      interactiveResizeRequested: null == interactiveResizeRequested
          ? _self.interactiveResizeRequested
          : interactiveResizeRequested // ignore: cast_nullable_to_non_nullable
              as ResizeEdgeObject,
      decoration: null == decoration
          ? _self.decoration
          : decoration // ignore: cast_nullable_to_non_nullable
              as ToplevelDecoration,
      title: null == title
          ? _self.title
          : title // ignore: cast_nullable_to_non_nullable
              as String,
      appId: null == appId
          ? _self.appId
          : appId // ignore: cast_nullable_to_non_nullable
              as String,
    ));
  }
}

/// Adds pattern-matching-related methods to [XdgToplevelState].
extension XdgToplevelStatePatterns on XdgToplevelState {
  /// A variant of `map` that fallback to returning `orElse`.
  ///
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case final Subclass value:
  ///     return ...;
  ///   case _:
  ///     return orElse();
  /// }
  /// ```

  @optionalTypeArgs
  TResult maybeMap<TResult extends Object?>(
    TResult Function(_XdgToplevelState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState() when $default != null:
        return $default(_that);
      case _:
        return orElse();
    }
  }

  /// A `switch`-like method, using callbacks.
  ///
  /// Callbacks receives the raw object, upcasted.
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case final Subclass value:
  ///     return ...;
  ///   case final Subclass2 value:
  ///     return ...;
  /// }
  /// ```

  @optionalTypeArgs
  TResult map<TResult extends Object?>(
    TResult Function(_XdgToplevelState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState():
        return $default(_that);
      case _:
        throw StateError('Unexpected subclass');
    }
  }

  /// A variant of `map` that fallback to returning `null`.
  ///
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case final Subclass value:
  ///     return ...;
  ///   case _:
  ///     return null;
  /// }
  /// ```

  @optionalTypeArgs
  TResult? mapOrNull<TResult extends Object?>(
    TResult? Function(_XdgToplevelState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState() when $default != null:
        return $default(_that);
      case _:
        return null;
    }
  }

  /// A variant of `when` that fallback to an `orElse` callback.
  ///
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case Subclass(:final field):
  ///     return ...;
  ///   case _:
  ///     return orElse();
  /// }
  /// ```

  @optionalTypeArgs
  TResult maybeWhen<TResult extends Object?>(
    TResult Function(
            bool visible,
            bool maximized,
            Key virtualKeyboardKey,
            FocusNode focusNode,
            Object interactiveMoveRequested,
            ResizeEdgeObject interactiveResizeRequested,
            ToplevelDecoration decoration,
            String title,
            String appId)?
        $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState() when $default != null:
        return $default(
            _that.visible,
            _that.maximized,
            _that.virtualKeyboardKey,
            _that.focusNode,
            _that.interactiveMoveRequested,
            _that.interactiveResizeRequested,
            _that.decoration,
            _that.title,
            _that.appId);
      case _:
        return orElse();
    }
  }

  /// A `switch`-like method, using callbacks.
  ///
  /// As opposed to `map`, this offers destructuring.
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case Subclass(:final field):
  ///     return ...;
  ///   case Subclass2(:final field2):
  ///     return ...;
  /// }
  /// ```

  @optionalTypeArgs
  TResult when<TResult extends Object?>(
    TResult Function(
            bool visible,
            bool maximized,
            Key virtualKeyboardKey,
            FocusNode focusNode,
            Object interactiveMoveRequested,
            ResizeEdgeObject interactiveResizeRequested,
            ToplevelDecoration decoration,
            String title,
            String appId)
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState():
        return $default(
            _that.visible,
            _that.maximized,
            _that.virtualKeyboardKey,
            _that.focusNode,
            _that.interactiveMoveRequested,
            _that.interactiveResizeRequested,
            _that.decoration,
            _that.title,
            _that.appId);
      case _:
        throw StateError('Unexpected subclass');
    }
  }

  /// A variant of `when` that fallback to returning `null`
  ///
  /// It is equivalent to doing:
  /// ```dart
  /// switch (sealedClass) {
  ///   case Subclass(:final field):
  ///     return ...;
  ///   case _:
  ///     return null;
  /// }
  /// ```

  @optionalTypeArgs
  TResult? whenOrNull<TResult extends Object?>(
    TResult? Function(
            bool visible,
            bool maximized,
            Key virtualKeyboardKey,
            FocusNode focusNode,
            Object interactiveMoveRequested,
            ResizeEdgeObject interactiveResizeRequested,
            ToplevelDecoration decoration,
            String title,
            String appId)?
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgToplevelState() when $default != null:
        return $default(
            _that.visible,
            _that.maximized,
            _that.virtualKeyboardKey,
            _that.focusNode,
            _that.interactiveMoveRequested,
            _that.interactiveResizeRequested,
            _that.decoration,
            _that.title,
            _that.appId);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _XdgToplevelState implements XdgToplevelState {
  const _XdgToplevelState(
      {required this.visible,
      required this.maximized,
      required this.virtualKeyboardKey,
      required this.focusNode,
      required this.interactiveMoveRequested,
      required this.interactiveResizeRequested,
      required this.decoration,
      required this.title,
      required this.appId});

  @override
  final bool visible;
  @override
  final bool maximized;
  @override
  final Key virtualKeyboardKey;
  @override
  final FocusNode focusNode;
  @override
  final Object interactiveMoveRequested;
  @override
  final ResizeEdgeObject interactiveResizeRequested;
  @override
  final ToplevelDecoration decoration;
  @override
  final String title;
  @override
  final String appId;

  /// Create a copy of XdgToplevelState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$XdgToplevelStateCopyWith<_XdgToplevelState> get copyWith =>
      __$XdgToplevelStateCopyWithImpl<_XdgToplevelState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _XdgToplevelState &&
            (identical(other.visible, visible) || other.visible == visible) &&
            (identical(other.maximized, maximized) ||
                other.maximized == maximized) &&
            (identical(other.virtualKeyboardKey, virtualKeyboardKey) ||
                other.virtualKeyboardKey == virtualKeyboardKey) &&
            (identical(other.focusNode, focusNode) ||
                other.focusNode == focusNode) &&
            const DeepCollectionEquality().equals(
                other.interactiveMoveRequested, interactiveMoveRequested) &&
            (identical(other.interactiveResizeRequested,
                    interactiveResizeRequested) ||
                other.interactiveResizeRequested ==
                    interactiveResizeRequested) &&
            (identical(other.decoration, decoration) ||
                other.decoration == decoration) &&
            (identical(other.title, title) || other.title == title) &&
            (identical(other.appId, appId) || other.appId == appId));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType,
      visible,
      maximized,
      virtualKeyboardKey,
      focusNode,
      const DeepCollectionEquality().hash(interactiveMoveRequested),
      interactiveResizeRequested,
      decoration,
      title,
      appId);

  @override
  String toString() {
    return 'XdgToplevelState(visible: $visible, maximized: $maximized, virtualKeyboardKey: $virtualKeyboardKey, focusNode: $focusNode, interactiveMoveRequested: $interactiveMoveRequested, interactiveResizeRequested: $interactiveResizeRequested, decoration: $decoration, title: $title, appId: $appId)';
  }
}

/// @nodoc
abstract mixin class _$XdgToplevelStateCopyWith<$Res>
    implements $XdgToplevelStateCopyWith<$Res> {
  factory _$XdgToplevelStateCopyWith(
          _XdgToplevelState value, $Res Function(_XdgToplevelState) _then) =
      __$XdgToplevelStateCopyWithImpl;
  @override
  @useResult
  $Res call(
      {bool visible,
      bool maximized,
      Key virtualKeyboardKey,
      FocusNode focusNode,
      Object interactiveMoveRequested,
      ResizeEdgeObject interactiveResizeRequested,
      ToplevelDecoration decoration,
      String title,
      String appId});
}

/// @nodoc
class __$XdgToplevelStateCopyWithImpl<$Res>
    implements _$XdgToplevelStateCopyWith<$Res> {
  __$XdgToplevelStateCopyWithImpl(this._self, this._then);

  final _XdgToplevelState _self;
  final $Res Function(_XdgToplevelState) _then;

  /// Create a copy of XdgToplevelState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? visible = null,
    Object? maximized = null,
    Object? virtualKeyboardKey = null,
    Object? focusNode = null,
    Object? interactiveMoveRequested = null,
    Object? interactiveResizeRequested = null,
    Object? decoration = null,
    Object? title = null,
    Object? appId = null,
  }) {
    return _then(_XdgToplevelState(
      visible: null == visible
          ? _self.visible
          : visible // ignore: cast_nullable_to_non_nullable
              as bool,
      maximized: null == maximized
          ? _self.maximized
          : maximized // ignore: cast_nullable_to_non_nullable
              as bool,
      virtualKeyboardKey: null == virtualKeyboardKey
          ? _self.virtualKeyboardKey
          : virtualKeyboardKey // ignore: cast_nullable_to_non_nullable
              as Key,
      focusNode: null == focusNode
          ? _self.focusNode
          : focusNode // ignore: cast_nullable_to_non_nullable
              as FocusNode,
      interactiveMoveRequested: null == interactiveMoveRequested
          ? _self.interactiveMoveRequested
          : interactiveMoveRequested,
      interactiveResizeRequested: null == interactiveResizeRequested
          ? _self.interactiveResizeRequested
          : interactiveResizeRequested // ignore: cast_nullable_to_non_nullable
              as ResizeEdgeObject,
      decoration: null == decoration
          ? _self.decoration
          : decoration // ignore: cast_nullable_to_non_nullable
              as ToplevelDecoration,
      title: null == title
          ? _self.title
          : title // ignore: cast_nullable_to_non_nullable
              as String,
      appId: null == appId
          ? _self.appId
          : appId // ignore: cast_nullable_to_non_nullable
              as String,
    ));
  }
}

// dart format on
