// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'xdg_surface_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$XdgSurfaceState {
  XdgSurfaceRole get role;
  Rect get visibleBounds;
  GlobalKey get widgetKey;
  List<int> get popups;

  /// Create a copy of XdgSurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $XdgSurfaceStateCopyWith<XdgSurfaceState> get copyWith =>
      _$XdgSurfaceStateCopyWithImpl<XdgSurfaceState>(
          this as XdgSurfaceState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is XdgSurfaceState &&
            (identical(other.role, role) || other.role == role) &&
            (identical(other.visibleBounds, visibleBounds) ||
                other.visibleBounds == visibleBounds) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey) &&
            const DeepCollectionEquality().equals(other.popups, popups));
  }

  @override
  int get hashCode => Object.hash(runtimeType, role, visibleBounds, widgetKey,
      const DeepCollectionEquality().hash(popups));

  @override
  String toString() {
    return 'XdgSurfaceState(role: $role, visibleBounds: $visibleBounds, widgetKey: $widgetKey, popups: $popups)';
  }
}

/// @nodoc
abstract mixin class $XdgSurfaceStateCopyWith<$Res> {
  factory $XdgSurfaceStateCopyWith(
          XdgSurfaceState value, $Res Function(XdgSurfaceState) _then) =
      _$XdgSurfaceStateCopyWithImpl;
  @useResult
  $Res call(
      {XdgSurfaceRole role,
      Rect visibleBounds,
      GlobalKey widgetKey,
      List<int> popups});
}

/// @nodoc
class _$XdgSurfaceStateCopyWithImpl<$Res>
    implements $XdgSurfaceStateCopyWith<$Res> {
  _$XdgSurfaceStateCopyWithImpl(this._self, this._then);

  final XdgSurfaceState _self;
  final $Res Function(XdgSurfaceState) _then;

  /// Create a copy of XdgSurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? role = null,
    Object? visibleBounds = null,
    Object? widgetKey = null,
    Object? popups = null,
  }) {
    return _then(_self.copyWith(
      role: null == role
          ? _self.role
          : role // ignore: cast_nullable_to_non_nullable
              as XdgSurfaceRole,
      visibleBounds: null == visibleBounds
          ? _self.visibleBounds
          : visibleBounds // ignore: cast_nullable_to_non_nullable
              as Rect,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      popups: null == popups
          ? _self.popups
          : popups // ignore: cast_nullable_to_non_nullable
              as List<int>,
    ));
  }
}

/// Adds pattern-matching-related methods to [XdgSurfaceState].
extension XdgSurfaceStatePatterns on XdgSurfaceState {
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
    TResult Function(_XdgSurfaceState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState() when $default != null:
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
    TResult Function(_XdgSurfaceState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState():
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
    TResult? Function(_XdgSurfaceState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState() when $default != null:
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
    TResult Function(XdgSurfaceRole role, Rect visibleBounds,
            GlobalKey widgetKey, List<int> popups)?
        $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState() when $default != null:
        return $default(
            _that.role, _that.visibleBounds, _that.widgetKey, _that.popups);
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
    TResult Function(XdgSurfaceRole role, Rect visibleBounds,
            GlobalKey widgetKey, List<int> popups)
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState():
        return $default(
            _that.role, _that.visibleBounds, _that.widgetKey, _that.popups);
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
    TResult? Function(XdgSurfaceRole role, Rect visibleBounds,
            GlobalKey widgetKey, List<int> popups)?
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _XdgSurfaceState() when $default != null:
        return $default(
            _that.role, _that.visibleBounds, _that.widgetKey, _that.popups);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _XdgSurfaceState implements XdgSurfaceState {
  const _XdgSurfaceState(
      {required this.role,
      required this.visibleBounds,
      required this.widgetKey,
      required final List<int> popups})
      : _popups = popups;

  @override
  final XdgSurfaceRole role;
  @override
  final Rect visibleBounds;
  @override
  final GlobalKey widgetKey;
  final List<int> _popups;
  @override
  List<int> get popups {
    if (_popups is EqualUnmodifiableListView) return _popups;
    // ignore: implicit_dynamic_type
    return EqualUnmodifiableListView(_popups);
  }

  /// Create a copy of XdgSurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$XdgSurfaceStateCopyWith<_XdgSurfaceState> get copyWith =>
      __$XdgSurfaceStateCopyWithImpl<_XdgSurfaceState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _XdgSurfaceState &&
            (identical(other.role, role) || other.role == role) &&
            (identical(other.visibleBounds, visibleBounds) ||
                other.visibleBounds == visibleBounds) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey) &&
            const DeepCollectionEquality().equals(other._popups, _popups));
  }

  @override
  int get hashCode => Object.hash(runtimeType, role, visibleBounds, widgetKey,
      const DeepCollectionEquality().hash(_popups));

  @override
  String toString() {
    return 'XdgSurfaceState(role: $role, visibleBounds: $visibleBounds, widgetKey: $widgetKey, popups: $popups)';
  }
}

/// @nodoc
abstract mixin class _$XdgSurfaceStateCopyWith<$Res>
    implements $XdgSurfaceStateCopyWith<$Res> {
  factory _$XdgSurfaceStateCopyWith(
          _XdgSurfaceState value, $Res Function(_XdgSurfaceState) _then) =
      __$XdgSurfaceStateCopyWithImpl;
  @override
  @useResult
  $Res call(
      {XdgSurfaceRole role,
      Rect visibleBounds,
      GlobalKey widgetKey,
      List<int> popups});
}

/// @nodoc
class __$XdgSurfaceStateCopyWithImpl<$Res>
    implements _$XdgSurfaceStateCopyWith<$Res> {
  __$XdgSurfaceStateCopyWithImpl(this._self, this._then);

  final _XdgSurfaceState _self;
  final $Res Function(_XdgSurfaceState) _then;

  /// Create a copy of XdgSurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? role = null,
    Object? visibleBounds = null,
    Object? widgetKey = null,
    Object? popups = null,
  }) {
    return _then(_XdgSurfaceState(
      role: null == role
          ? _self.role
          : role // ignore: cast_nullable_to_non_nullable
              as XdgSurfaceRole,
      visibleBounds: null == visibleBounds
          ? _self.visibleBounds
          : visibleBounds // ignore: cast_nullable_to_non_nullable
              as Rect,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      popups: null == popups
          ? _self._popups
          : popups // ignore: cast_nullable_to_non_nullable
              as List<int>,
    ));
  }
}

// dart format on
