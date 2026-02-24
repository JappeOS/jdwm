// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'window_stack_provider.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$WindowStackState {
  List<int> get stack;

  /// Create a copy of WindowStackState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $WindowStackStateCopyWith<WindowStackState> get copyWith =>
      _$WindowStackStateCopyWithImpl<WindowStackState>(
          this as WindowStackState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is WindowStackState &&
            const DeepCollectionEquality().equals(other.stack, stack));
  }

  @override
  int get hashCode =>
      Object.hash(runtimeType, const DeepCollectionEquality().hash(stack));

  @override
  String toString() {
    return 'WindowStackState(stack: $stack)';
  }
}

/// @nodoc
abstract mixin class $WindowStackStateCopyWith<$Res> {
  factory $WindowStackStateCopyWith(
          WindowStackState value, $Res Function(WindowStackState) _then) =
      _$WindowStackStateCopyWithImpl;
  @useResult
  $Res call({List<int> stack});
}

/// @nodoc
class _$WindowStackStateCopyWithImpl<$Res>
    implements $WindowStackStateCopyWith<$Res> {
  _$WindowStackStateCopyWithImpl(this._self, this._then);

  final WindowStackState _self;
  final $Res Function(WindowStackState) _then;

  /// Create a copy of WindowStackState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? stack = null,
  }) {
    return _then(_self.copyWith(
      stack: null == stack
          ? _self.stack
          : stack // ignore: cast_nullable_to_non_nullable
              as List<int>,
    ));
  }
}

/// Adds pattern-matching-related methods to [WindowStackState].
extension WindowStackStatePatterns on WindowStackState {
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
    TResult Function(_WindowStackState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _WindowStackState() when $default != null:
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
    TResult Function(_WindowStackState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _WindowStackState():
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
    TResult? Function(_WindowStackState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _WindowStackState() when $default != null:
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
    TResult Function(List<int> stack)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _WindowStackState() when $default != null:
        return $default(_that.stack);
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
    TResult Function(List<int> stack) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _WindowStackState():
        return $default(_that.stack);
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
    TResult? Function(List<int> stack)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _WindowStackState() when $default != null:
        return $default(_that.stack);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _WindowStackState extends WindowStackState {
  const _WindowStackState({required final List<int> stack})
      : _stack = stack,
        super._();

  final List<int> _stack;
  @override
  List<int> get stack {
    if (_stack is EqualUnmodifiableListView) return _stack;
    // ignore: implicit_dynamic_type
    return EqualUnmodifiableListView(_stack);
  }

  /// Create a copy of WindowStackState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$WindowStackStateCopyWith<_WindowStackState> get copyWith =>
      __$WindowStackStateCopyWithImpl<_WindowStackState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _WindowStackState &&
            const DeepCollectionEquality().equals(other._stack, _stack));
  }

  @override
  int get hashCode =>
      Object.hash(runtimeType, const DeepCollectionEquality().hash(_stack));

  @override
  String toString() {
    return 'WindowStackState(stack: $stack)';
  }
}

/// @nodoc
abstract mixin class _$WindowStackStateCopyWith<$Res>
    implements $WindowStackStateCopyWith<$Res> {
  factory _$WindowStackStateCopyWith(
          _WindowStackState value, $Res Function(_WindowStackState) _then) =
      __$WindowStackStateCopyWithImpl;
  @override
  @useResult
  $Res call({List<int> stack});
}

/// @nodoc
class __$WindowStackStateCopyWithImpl<$Res>
    implements _$WindowStackStateCopyWith<$Res> {
  __$WindowStackStateCopyWithImpl(this._self, this._then);

  final _WindowStackState _self;
  final $Res Function(_WindowStackState) _then;

  /// Create a copy of WindowStackState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? stack = null,
  }) {
    return _then(_WindowStackState(
      stack: null == stack
          ? _self._stack
          : stack // ignore: cast_nullable_to_non_nullable
              as List<int>,
    ));
  }
}

// dart format on
