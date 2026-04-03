// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'virtual_keyboard_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$VirtualKeyboardState {
  bool get activated;
  Size get size;

  /// Create a copy of VirtualKeyboardState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $VirtualKeyboardStateCopyWith<VirtualKeyboardState> get copyWith =>
      _$VirtualKeyboardStateCopyWithImpl<VirtualKeyboardState>(
          this as VirtualKeyboardState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is VirtualKeyboardState &&
            (identical(other.activated, activated) ||
                other.activated == activated) &&
            (identical(other.size, size) || other.size == size));
  }

  @override
  int get hashCode => Object.hash(runtimeType, activated, size);

  @override
  String toString() {
    return 'VirtualKeyboardState(activated: $activated, size: $size)';
  }
}

/// @nodoc
abstract mixin class $VirtualKeyboardStateCopyWith<$Res> {
  factory $VirtualKeyboardStateCopyWith(VirtualKeyboardState value,
          $Res Function(VirtualKeyboardState) _then) =
      _$VirtualKeyboardStateCopyWithImpl;
  @useResult
  $Res call({bool activated, Size size});
}

/// @nodoc
class _$VirtualKeyboardStateCopyWithImpl<$Res>
    implements $VirtualKeyboardStateCopyWith<$Res> {
  _$VirtualKeyboardStateCopyWithImpl(this._self, this._then);

  final VirtualKeyboardState _self;
  final $Res Function(VirtualKeyboardState) _then;

  /// Create a copy of VirtualKeyboardState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? activated = null,
    Object? size = null,
  }) {
    return _then(_self.copyWith(
      activated: null == activated
          ? _self.activated
          : activated // ignore: cast_nullable_to_non_nullable
              as bool,
      size: null == size
          ? _self.size
          : size // ignore: cast_nullable_to_non_nullable
              as Size,
    ));
  }
}

/// Adds pattern-matching-related methods to [VirtualKeyboardState].
extension VirtualKeyboardStatePatterns on VirtualKeyboardState {
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
    TResult Function(_VirtualKeyboardState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState() when $default != null:
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
    TResult Function(_VirtualKeyboardState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState():
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
    TResult? Function(_VirtualKeyboardState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState() when $default != null:
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
    TResult Function(bool activated, Size size)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState() when $default != null:
        return $default(_that.activated, _that.size);
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
    TResult Function(bool activated, Size size) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState():
        return $default(_that.activated, _that.size);
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
    TResult? Function(bool activated, Size size)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _VirtualKeyboardState() when $default != null:
        return $default(_that.activated, _that.size);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _VirtualKeyboardState implements VirtualKeyboardState {
  const _VirtualKeyboardState({required this.activated, required this.size});

  @override
  final bool activated;
  @override
  final Size size;

  /// Create a copy of VirtualKeyboardState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$VirtualKeyboardStateCopyWith<_VirtualKeyboardState> get copyWith =>
      __$VirtualKeyboardStateCopyWithImpl<_VirtualKeyboardState>(
          this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _VirtualKeyboardState &&
            (identical(other.activated, activated) ||
                other.activated == activated) &&
            (identical(other.size, size) || other.size == size));
  }

  @override
  int get hashCode => Object.hash(runtimeType, activated, size);

  @override
  String toString() {
    return 'VirtualKeyboardState(activated: $activated, size: $size)';
  }
}

/// @nodoc
abstract mixin class _$VirtualKeyboardStateCopyWith<$Res>
    implements $VirtualKeyboardStateCopyWith<$Res> {
  factory _$VirtualKeyboardStateCopyWith(_VirtualKeyboardState value,
          $Res Function(_VirtualKeyboardState) _then) =
      __$VirtualKeyboardStateCopyWithImpl;
  @override
  @useResult
  $Res call({bool activated, Size size});
}

/// @nodoc
class __$VirtualKeyboardStateCopyWithImpl<$Res>
    implements _$VirtualKeyboardStateCopyWith<$Res> {
  __$VirtualKeyboardStateCopyWithImpl(this._self, this._then);

  final _VirtualKeyboardState _self;
  final $Res Function(_VirtualKeyboardState) _then;

  /// Create a copy of VirtualKeyboardState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? activated = null,
    Object? size = null,
  }) {
    return _then(_VirtualKeyboardState(
      activated: null == activated
          ? _self.activated
          : activated // ignore: cast_nullable_to_non_nullable
              as bool,
      size: null == size
          ? _self.size
          : size // ignore: cast_nullable_to_non_nullable
              as Size,
    ));
  }
}

// dart format on
