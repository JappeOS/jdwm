// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'xdg_popup_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$XdgPopupState implements DiagnosticableTreeMixin {
  int get parentViewId;
  Offset get position;
  GlobalKey<AnimationsState> get animationsKey;
  bool get isClosing;

  /// Create a copy of XdgPopupState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $XdgPopupStateCopyWith<XdgPopupState> get copyWith =>
      _$XdgPopupStateCopyWithImpl<XdgPopupState>(
          this as XdgPopupState, _$identity);

  @override
  void debugFillProperties(DiagnosticPropertiesBuilder properties) {
    properties
      ..add(DiagnosticsProperty('type', 'XdgPopupState'))
      ..add(DiagnosticsProperty('parentViewId', parentViewId))
      ..add(DiagnosticsProperty('position', position))
      ..add(DiagnosticsProperty('animationsKey', animationsKey))
      ..add(DiagnosticsProperty('isClosing', isClosing));
  }

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is XdgPopupState &&
            (identical(other.parentViewId, parentViewId) ||
                other.parentViewId == parentViewId) &&
            (identical(other.position, position) ||
                other.position == position) &&
            (identical(other.animationsKey, animationsKey) ||
                other.animationsKey == animationsKey) &&
            (identical(other.isClosing, isClosing) ||
                other.isClosing == isClosing));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType, parentViewId, position, animationsKey, isClosing);

  @override
  String toString({DiagnosticLevel minLevel = DiagnosticLevel.info}) {
    return 'XdgPopupState(parentViewId: $parentViewId, position: $position, animationsKey: $animationsKey, isClosing: $isClosing)';
  }
}

/// @nodoc
abstract mixin class $XdgPopupStateCopyWith<$Res> {
  factory $XdgPopupStateCopyWith(
          XdgPopupState value, $Res Function(XdgPopupState) _then) =
      _$XdgPopupStateCopyWithImpl;
  @useResult
  $Res call(
      {int parentViewId,
      Offset position,
      GlobalKey<AnimationsState> animationsKey,
      bool isClosing});
}

/// @nodoc
class _$XdgPopupStateCopyWithImpl<$Res>
    implements $XdgPopupStateCopyWith<$Res> {
  _$XdgPopupStateCopyWithImpl(this._self, this._then);

  final XdgPopupState _self;
  final $Res Function(XdgPopupState) _then;

  /// Create a copy of XdgPopupState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? parentViewId = null,
    Object? position = null,
    Object? animationsKey = null,
    Object? isClosing = null,
  }) {
    return _then(_self.copyWith(
      parentViewId: null == parentViewId
          ? _self.parentViewId
          : parentViewId // ignore: cast_nullable_to_non_nullable
              as int,
      position: null == position
          ? _self.position
          : position // ignore: cast_nullable_to_non_nullable
              as Offset,
      animationsKey: null == animationsKey
          ? _self.animationsKey
          : animationsKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey<AnimationsState>,
      isClosing: null == isClosing
          ? _self.isClosing
          : isClosing // ignore: cast_nullable_to_non_nullable
              as bool,
    ));
  }
}

/// Adds pattern-matching-related methods to [XdgPopupState].
extension XdgPopupStatePatterns on XdgPopupState {
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
    TResult Function(_XdgPopupState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState() when $default != null:
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
    TResult Function(_XdgPopupState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState():
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
    TResult? Function(_XdgPopupState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState() when $default != null:
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
    TResult Function(int parentViewId, Offset position,
            GlobalKey<AnimationsState> animationsKey, bool isClosing)?
        $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState() when $default != null:
        return $default(_that.parentViewId, _that.position, _that.animationsKey,
            _that.isClosing);
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
    TResult Function(int parentViewId, Offset position,
            GlobalKey<AnimationsState> animationsKey, bool isClosing)
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState():
        return $default(_that.parentViewId, _that.position, _that.animationsKey,
            _that.isClosing);
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
    TResult? Function(int parentViewId, Offset position,
            GlobalKey<AnimationsState> animationsKey, bool isClosing)?
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgPopupState() when $default != null:
        return $default(_that.parentViewId, _that.position, _that.animationsKey,
            _that.isClosing);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _XdgPopupState with DiagnosticableTreeMixin implements XdgPopupState {
  const _XdgPopupState(
      {required this.parentViewId,
      required this.position,
      required this.animationsKey,
      required this.isClosing});

  @override
  final int parentViewId;
  @override
  final Offset position;
  @override
  final GlobalKey<AnimationsState> animationsKey;
  @override
  final bool isClosing;

  /// Create a copy of XdgPopupState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$XdgPopupStateCopyWith<_XdgPopupState> get copyWith =>
      __$XdgPopupStateCopyWithImpl<_XdgPopupState>(this, _$identity);

  @override
  void debugFillProperties(DiagnosticPropertiesBuilder properties) {
    properties
      ..add(DiagnosticsProperty('type', 'XdgPopupState'))
      ..add(DiagnosticsProperty('parentViewId', parentViewId))
      ..add(DiagnosticsProperty('position', position))
      ..add(DiagnosticsProperty('animationsKey', animationsKey))
      ..add(DiagnosticsProperty('isClosing', isClosing));
  }

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _XdgPopupState &&
            (identical(other.parentViewId, parentViewId) ||
                other.parentViewId == parentViewId) &&
            (identical(other.position, position) ||
                other.position == position) &&
            (identical(other.animationsKey, animationsKey) ||
                other.animationsKey == animationsKey) &&
            (identical(other.isClosing, isClosing) ||
                other.isClosing == isClosing));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType, parentViewId, position, animationsKey, isClosing);

  @override
  String toString({DiagnosticLevel minLevel = DiagnosticLevel.info}) {
    return 'XdgPopupState(parentViewId: $parentViewId, position: $position, animationsKey: $animationsKey, isClosing: $isClosing)';
  }
}

/// @nodoc
abstract mixin class _$XdgPopupStateCopyWith<$Res>
    implements $XdgPopupStateCopyWith<$Res> {
  factory _$XdgPopupStateCopyWith(
          _XdgPopupState value, $Res Function(_XdgPopupState) _then) =
      __$XdgPopupStateCopyWithImpl;
  @override
  @useResult
  $Res call(
      {int parentViewId,
      Offset position,
      GlobalKey<AnimationsState> animationsKey,
      bool isClosing});
}

/// @nodoc
class __$XdgPopupStateCopyWithImpl<$Res>
    implements _$XdgPopupStateCopyWith<$Res> {
  __$XdgPopupStateCopyWithImpl(this._self, this._then);

  final _XdgPopupState _self;
  final $Res Function(_XdgPopupState) _then;

  /// Create a copy of XdgPopupState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? parentViewId = null,
    Object? position = null,
    Object? animationsKey = null,
    Object? isClosing = null,
  }) {
    return _then(_XdgPopupState(
      parentViewId: null == parentViewId
          ? _self.parentViewId
          : parentViewId // ignore: cast_nullable_to_non_nullable
              as int,
      position: null == position
          ? _self.position
          : position // ignore: cast_nullable_to_non_nullable
              as Offset,
      animationsKey: null == animationsKey
          ? _self.animationsKey
          : animationsKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey<AnimationsState>,
      isClosing: null == isClosing
          ? _self.isClosing
          : isClosing // ignore: cast_nullable_to_non_nullable
              as bool,
    ));
  }
}

// dart format on
