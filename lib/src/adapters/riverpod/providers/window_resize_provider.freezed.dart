// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'window_resize_provider.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$ResizerState {
  bool get resizing;
  ResizeEdge? get resizeEdge;
  Size get startSize;
  Size get wantedSize;
  Offset get delta;

  /// Create a copy of ResizerState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $ResizerStateCopyWith<ResizerState> get copyWith =>
      _$ResizerStateCopyWithImpl<ResizerState>(
          this as ResizerState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is ResizerState &&
            (identical(other.resizing, resizing) ||
                other.resizing == resizing) &&
            (identical(other.resizeEdge, resizeEdge) ||
                other.resizeEdge == resizeEdge) &&
            (identical(other.startSize, startSize) ||
                other.startSize == startSize) &&
            (identical(other.wantedSize, wantedSize) ||
                other.wantedSize == wantedSize) &&
            (identical(other.delta, delta) || other.delta == delta));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType, resizing, resizeEdge, startSize, wantedSize, delta);

  @override
  String toString() {
    return 'ResizerState(resizing: $resizing, resizeEdge: $resizeEdge, startSize: $startSize, wantedSize: $wantedSize, delta: $delta)';
  }
}

/// @nodoc
abstract mixin class $ResizerStateCopyWith<$Res> {
  factory $ResizerStateCopyWith(
          ResizerState value, $Res Function(ResizerState) _then) =
      _$ResizerStateCopyWithImpl;
  @useResult
  $Res call(
      {bool resizing,
      ResizeEdge? resizeEdge,
      Size startSize,
      Size wantedSize,
      Offset delta});
}

/// @nodoc
class _$ResizerStateCopyWithImpl<$Res> implements $ResizerStateCopyWith<$Res> {
  _$ResizerStateCopyWithImpl(this._self, this._then);

  final ResizerState _self;
  final $Res Function(ResizerState) _then;

  /// Create a copy of ResizerState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? resizing = null,
    Object? resizeEdge = freezed,
    Object? startSize = null,
    Object? wantedSize = null,
    Object? delta = null,
  }) {
    return _then(_self.copyWith(
      resizing: null == resizing
          ? _self.resizing
          : resizing // ignore: cast_nullable_to_non_nullable
              as bool,
      resizeEdge: freezed == resizeEdge
          ? _self.resizeEdge
          : resizeEdge // ignore: cast_nullable_to_non_nullable
              as ResizeEdge?,
      startSize: null == startSize
          ? _self.startSize
          : startSize // ignore: cast_nullable_to_non_nullable
              as Size,
      wantedSize: null == wantedSize
          ? _self.wantedSize
          : wantedSize // ignore: cast_nullable_to_non_nullable
              as Size,
      delta: null == delta
          ? _self.delta
          : delta // ignore: cast_nullable_to_non_nullable
              as Offset,
    ));
  }
}

/// Adds pattern-matching-related methods to [ResizerState].
extension ResizerStatePatterns on ResizerState {
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
    TResult Function(_ResizerState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _ResizerState() when $default != null:
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
    TResult Function(_ResizerState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _ResizerState():
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
    TResult? Function(_ResizerState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _ResizerState() when $default != null:
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
    TResult Function(bool resizing, ResizeEdge? resizeEdge, Size startSize,
            Size wantedSize, Offset delta)?
        $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _ResizerState() when $default != null:
        return $default(_that.resizing, _that.resizeEdge, _that.startSize,
            _that.wantedSize, _that.delta);
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
    TResult Function(bool resizing, ResizeEdge? resizeEdge, Size startSize,
            Size wantedSize, Offset delta)
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _ResizerState():
        return $default(_that.resizing, _that.resizeEdge, _that.startSize,
            _that.wantedSize, _that.delta);
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
    TResult? Function(bool resizing, ResizeEdge? resizeEdge, Size startSize,
            Size wantedSize, Offset delta)?
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _ResizerState() when $default != null:
        return $default(_that.resizing, _that.resizeEdge, _that.startSize,
            _that.wantedSize, _that.delta);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _ResizerState implements ResizerState {
  const _ResizerState(
      {required this.resizing,
      required this.resizeEdge,
      required this.startSize,
      required this.wantedSize,
      required this.delta});

  @override
  final bool resizing;
  @override
  final ResizeEdge? resizeEdge;
  @override
  final Size startSize;
  @override
  final Size wantedSize;
  @override
  final Offset delta;

  /// Create a copy of ResizerState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$ResizerStateCopyWith<_ResizerState> get copyWith =>
      __$ResizerStateCopyWithImpl<_ResizerState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _ResizerState &&
            (identical(other.resizing, resizing) ||
                other.resizing == resizing) &&
            (identical(other.resizeEdge, resizeEdge) ||
                other.resizeEdge == resizeEdge) &&
            (identical(other.startSize, startSize) ||
                other.startSize == startSize) &&
            (identical(other.wantedSize, wantedSize) ||
                other.wantedSize == wantedSize) &&
            (identical(other.delta, delta) || other.delta == delta));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType, resizing, resizeEdge, startSize, wantedSize, delta);

  @override
  String toString() {
    return 'ResizerState(resizing: $resizing, resizeEdge: $resizeEdge, startSize: $startSize, wantedSize: $wantedSize, delta: $delta)';
  }
}

/// @nodoc
abstract mixin class _$ResizerStateCopyWith<$Res>
    implements $ResizerStateCopyWith<$Res> {
  factory _$ResizerStateCopyWith(
          _ResizerState value, $Res Function(_ResizerState) _then) =
      __$ResizerStateCopyWithImpl;
  @override
  @useResult
  $Res call(
      {bool resizing,
      ResizeEdge? resizeEdge,
      Size startSize,
      Size wantedSize,
      Offset delta});
}

/// @nodoc
class __$ResizerStateCopyWithImpl<$Res>
    implements _$ResizerStateCopyWith<$Res> {
  __$ResizerStateCopyWithImpl(this._self, this._then);

  final _ResizerState _self;
  final $Res Function(_ResizerState) _then;

  /// Create a copy of ResizerState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? resizing = null,
    Object? resizeEdge = freezed,
    Object? startSize = null,
    Object? wantedSize = null,
    Object? delta = null,
  }) {
    return _then(_ResizerState(
      resizing: null == resizing
          ? _self.resizing
          : resizing // ignore: cast_nullable_to_non_nullable
              as bool,
      resizeEdge: freezed == resizeEdge
          ? _self.resizeEdge
          : resizeEdge // ignore: cast_nullable_to_non_nullable
              as ResizeEdge?,
      startSize: null == startSize
          ? _self.startSize
          : startSize // ignore: cast_nullable_to_non_nullable
              as Size,
      wantedSize: null == wantedSize
          ? _self.wantedSize
          : wantedSize // ignore: cast_nullable_to_non_nullable
              as Size,
      delta: null == delta
          ? _self.delta
          : delta // ignore: cast_nullable_to_non_nullable
              as Offset,
    ));
  }
}

// dart format on
