// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'subsurface_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$SubsurfaceState {
  Offset get position; // relative to the parent
  bool get mapped;
  Key get widgetKey;

  /// Create a copy of SubsurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $SubsurfaceStateCopyWith<SubsurfaceState> get copyWith =>
      _$SubsurfaceStateCopyWithImpl<SubsurfaceState>(
          this as SubsurfaceState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is SubsurfaceState &&
            (identical(other.position, position) ||
                other.position == position) &&
            (identical(other.mapped, mapped) || other.mapped == mapped) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey));
  }

  @override
  int get hashCode => Object.hash(runtimeType, position, mapped, widgetKey);

  @override
  String toString() {
    return 'SubsurfaceState(position: $position, mapped: $mapped, widgetKey: $widgetKey)';
  }
}

/// @nodoc
abstract mixin class $SubsurfaceStateCopyWith<$Res> {
  factory $SubsurfaceStateCopyWith(
          SubsurfaceState value, $Res Function(SubsurfaceState) _then) =
      _$SubsurfaceStateCopyWithImpl;
  @useResult
  $Res call({Offset position, bool mapped, Key widgetKey});
}

/// @nodoc
class _$SubsurfaceStateCopyWithImpl<$Res>
    implements $SubsurfaceStateCopyWith<$Res> {
  _$SubsurfaceStateCopyWithImpl(this._self, this._then);

  final SubsurfaceState _self;
  final $Res Function(SubsurfaceState) _then;

  /// Create a copy of SubsurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? position = null,
    Object? mapped = null,
    Object? widgetKey = null,
  }) {
    return _then(_self.copyWith(
      position: null == position
          ? _self.position
          : position // ignore: cast_nullable_to_non_nullable
              as Offset,
      mapped: null == mapped
          ? _self.mapped
          : mapped // ignore: cast_nullable_to_non_nullable
              as bool,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as Key,
    ));
  }
}

/// Adds pattern-matching-related methods to [SubsurfaceState].
extension SubsurfaceStatePatterns on SubsurfaceState {
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
    TResult Function(_SubsurfaceState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState() when $default != null:
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
    TResult Function(_SubsurfaceState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState():
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
    TResult? Function(_SubsurfaceState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState() when $default != null:
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
    TResult Function(Offset position, bool mapped, Key widgetKey)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState() when $default != null:
        return $default(_that.position, _that.mapped, _that.widgetKey);
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
    TResult Function(Offset position, bool mapped, Key widgetKey) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState():
        return $default(_that.position, _that.mapped, _that.widgetKey);
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
    TResult? Function(Offset position, bool mapped, Key widgetKey)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SubsurfaceState() when $default != null:
        return $default(_that.position, _that.mapped, _that.widgetKey);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _SubsurfaceState implements SubsurfaceState {
  const _SubsurfaceState(
      {required this.position, required this.mapped, required this.widgetKey});

  @override
  final Offset position;
// relative to the parent
  @override
  final bool mapped;
  @override
  final Key widgetKey;

  /// Create a copy of SubsurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$SubsurfaceStateCopyWith<_SubsurfaceState> get copyWith =>
      __$SubsurfaceStateCopyWithImpl<_SubsurfaceState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _SubsurfaceState &&
            (identical(other.position, position) ||
                other.position == position) &&
            (identical(other.mapped, mapped) || other.mapped == mapped) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey));
  }

  @override
  int get hashCode => Object.hash(runtimeType, position, mapped, widgetKey);

  @override
  String toString() {
    return 'SubsurfaceState(position: $position, mapped: $mapped, widgetKey: $widgetKey)';
  }
}

/// @nodoc
abstract mixin class _$SubsurfaceStateCopyWith<$Res>
    implements $SubsurfaceStateCopyWith<$Res> {
  factory _$SubsurfaceStateCopyWith(
          _SubsurfaceState value, $Res Function(_SubsurfaceState) _then) =
      __$SubsurfaceStateCopyWithImpl;
  @override
  @useResult
  $Res call({Offset position, bool mapped, Key widgetKey});
}

/// @nodoc
class __$SubsurfaceStateCopyWithImpl<$Res>
    implements _$SubsurfaceStateCopyWith<$Res> {
  __$SubsurfaceStateCopyWithImpl(this._self, this._then);

  final _SubsurfaceState _self;
  final $Res Function(_SubsurfaceState) _then;

  /// Create a copy of SubsurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? position = null,
    Object? mapped = null,
    Object? widgetKey = null,
  }) {
    return _then(_SubsurfaceState(
      position: null == position
          ? _self.position
          : position // ignore: cast_nullable_to_non_nullable
              as Offset,
      mapped: null == mapped
          ? _self.mapped
          : mapped // ignore: cast_nullable_to_non_nullable
              as bool,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as Key,
    ));
  }
}

// dart format on
