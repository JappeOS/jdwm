// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'surface_state.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$SurfaceState {
  SurfaceRole get role;
  int get viewId;
  int get textureId;
  Offset get surfacePosition;
  Size get surfaceSize;
  double get scale;
  GlobalKey get widgetKey;
  GlobalKey get textureKey;
  List<int> get subsurfacesBelow;
  List<int> get subsurfacesAbove;
  Rect get inputRegion;

  /// Create a copy of SurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  $SurfaceStateCopyWith<SurfaceState> get copyWith =>
      _$SurfaceStateCopyWithImpl<SurfaceState>(
          this as SurfaceState, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is SurfaceState &&
            (identical(other.role, role) || other.role == role) &&
            (identical(other.viewId, viewId) || other.viewId == viewId) &&
            (identical(other.textureId, textureId) ||
                other.textureId == textureId) &&
            (identical(other.surfacePosition, surfacePosition) ||
                other.surfacePosition == surfacePosition) &&
            (identical(other.surfaceSize, surfaceSize) ||
                other.surfaceSize == surfaceSize) &&
            (identical(other.scale, scale) || other.scale == scale) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey) &&
            (identical(other.textureKey, textureKey) ||
                other.textureKey == textureKey) &&
            const DeepCollectionEquality()
                .equals(other.subsurfacesBelow, subsurfacesBelow) &&
            const DeepCollectionEquality()
                .equals(other.subsurfacesAbove, subsurfacesAbove) &&
            (identical(other.inputRegion, inputRegion) ||
                other.inputRegion == inputRegion));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType,
      role,
      viewId,
      textureId,
      surfacePosition,
      surfaceSize,
      scale,
      widgetKey,
      textureKey,
      const DeepCollectionEquality().hash(subsurfacesBelow),
      const DeepCollectionEquality().hash(subsurfacesAbove),
      inputRegion);

  @override
  String toString() {
    return 'SurfaceState(role: $role, viewId: $viewId, textureId: $textureId, surfacePosition: $surfacePosition, surfaceSize: $surfaceSize, scale: $scale, widgetKey: $widgetKey, textureKey: $textureKey, subsurfacesBelow: $subsurfacesBelow, subsurfacesAbove: $subsurfacesAbove, inputRegion: $inputRegion)';
  }
}

/// @nodoc
abstract mixin class $SurfaceStateCopyWith<$Res> {
  factory $SurfaceStateCopyWith(
          SurfaceState value, $Res Function(SurfaceState) _then) =
      _$SurfaceStateCopyWithImpl;
  @useResult
  $Res call(
      {SurfaceRole role,
      int viewId,
      int textureId,
      Offset surfacePosition,
      Size surfaceSize,
      double scale,
      GlobalKey widgetKey,
      GlobalKey textureKey,
      List<int> subsurfacesBelow,
      List<int> subsurfacesAbove,
      Rect inputRegion});
}

/// @nodoc
class _$SurfaceStateCopyWithImpl<$Res> implements $SurfaceStateCopyWith<$Res> {
  _$SurfaceStateCopyWithImpl(this._self, this._then);

  final SurfaceState _self;
  final $Res Function(SurfaceState) _then;

  /// Create a copy of SurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @pragma('vm:prefer-inline')
  @override
  $Res call({
    Object? role = null,
    Object? viewId = null,
    Object? textureId = null,
    Object? surfacePosition = null,
    Object? surfaceSize = null,
    Object? scale = null,
    Object? widgetKey = null,
    Object? textureKey = null,
    Object? subsurfacesBelow = null,
    Object? subsurfacesAbove = null,
    Object? inputRegion = null,
  }) {
    return _then(_self.copyWith(
      role: null == role
          ? _self.role
          : role // ignore: cast_nullable_to_non_nullable
              as SurfaceRole,
      viewId: null == viewId
          ? _self.viewId
          : viewId // ignore: cast_nullable_to_non_nullable
              as int,
      textureId: null == textureId
          ? _self.textureId
          : textureId // ignore: cast_nullable_to_non_nullable
              as int,
      surfacePosition: null == surfacePosition
          ? _self.surfacePosition
          : surfacePosition // ignore: cast_nullable_to_non_nullable
              as Offset,
      surfaceSize: null == surfaceSize
          ? _self.surfaceSize
          : surfaceSize // ignore: cast_nullable_to_non_nullable
              as Size,
      scale: null == scale
          ? _self.scale
          : scale // ignore: cast_nullable_to_non_nullable
              as double,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      textureKey: null == textureKey
          ? _self.textureKey
          : textureKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      subsurfacesBelow: null == subsurfacesBelow
          ? _self.subsurfacesBelow
          : subsurfacesBelow // ignore: cast_nullable_to_non_nullable
              as List<int>,
      subsurfacesAbove: null == subsurfacesAbove
          ? _self.subsurfacesAbove
          : subsurfacesAbove // ignore: cast_nullable_to_non_nullable
              as List<int>,
      inputRegion: null == inputRegion
          ? _self.inputRegion
          : inputRegion // ignore: cast_nullable_to_non_nullable
              as Rect,
    ));
  }
}

/// Adds pattern-matching-related methods to [SurfaceState].
extension SurfaceStatePatterns on SurfaceState {
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
    TResult Function(_SurfaceState value)? $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _SurfaceState() when $default != null:
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
    TResult Function(_SurfaceState value) $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SurfaceState():
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
    TResult? Function(_SurfaceState value)? $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SurfaceState() when $default != null:
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
            SurfaceRole role,
            int viewId,
            int textureId,
            Offset surfacePosition,
            Size surfaceSize,
            double scale,
            GlobalKey widgetKey,
            GlobalKey textureKey,
            List<int> subsurfacesBelow,
            List<int> subsurfacesAbove,
            Rect inputRegion)?
        $default, {
    required TResult orElse(),
  }) {
    final _that = this;
    switch (_that) {
      case _SurfaceState() when $default != null:
        return $default(
            _that.role,
            _that.viewId,
            _that.textureId,
            _that.surfacePosition,
            _that.surfaceSize,
            _that.scale,
            _that.widgetKey,
            _that.textureKey,
            _that.subsurfacesBelow,
            _that.subsurfacesAbove,
            _that.inputRegion);
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
            SurfaceRole role,
            int viewId,
            int textureId,
            Offset surfacePosition,
            Size surfaceSize,
            double scale,
            GlobalKey widgetKey,
            GlobalKey textureKey,
            List<int> subsurfacesBelow,
            List<int> subsurfacesAbove,
            Rect inputRegion)
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SurfaceState():
        return $default(
            _that.role,
            _that.viewId,
            _that.textureId,
            _that.surfacePosition,
            _that.surfaceSize,
            _that.scale,
            _that.widgetKey,
            _that.textureKey,
            _that.subsurfacesBelow,
            _that.subsurfacesAbove,
            _that.inputRegion);
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
            SurfaceRole role,
            int viewId,
            int textureId,
            Offset surfacePosition,
            Size surfaceSize,
            double scale,
            GlobalKey widgetKey,
            GlobalKey textureKey,
            List<int> subsurfacesBelow,
            List<int> subsurfacesAbove,
            Rect inputRegion)?
        $default,
  ) {
    final _that = this;
    switch (_that) {
      case _SurfaceState() when $default != null:
        return $default(
            _that.role,
            _that.viewId,
            _that.textureId,
            _that.surfacePosition,
            _that.surfaceSize,
            _that.scale,
            _that.widgetKey,
            _that.textureKey,
            _that.subsurfacesBelow,
            _that.subsurfacesAbove,
            _that.inputRegion);
      case _:
        return null;
    }
  }
}

/// @nodoc

class _SurfaceState implements SurfaceState {
  const _SurfaceState(
      {required this.role,
      required this.viewId,
      required this.textureId,
      required this.surfacePosition,
      required this.surfaceSize,
      required this.scale,
      required this.widgetKey,
      required this.textureKey,
      required final List<int> subsurfacesBelow,
      required final List<int> subsurfacesAbove,
      required this.inputRegion})
      : _subsurfacesBelow = subsurfacesBelow,
        _subsurfacesAbove = subsurfacesAbove;

  @override
  final SurfaceRole role;
  @override
  final int viewId;
  @override
  final int textureId;
  @override
  final Offset surfacePosition;
  @override
  final Size surfaceSize;
  @override
  final double scale;
  @override
  final GlobalKey widgetKey;
  @override
  final GlobalKey textureKey;
  final List<int> _subsurfacesBelow;
  @override
  List<int> get subsurfacesBelow {
    if (_subsurfacesBelow is EqualUnmodifiableListView)
      return _subsurfacesBelow;
    // ignore: implicit_dynamic_type
    return EqualUnmodifiableListView(_subsurfacesBelow);
  }

  final List<int> _subsurfacesAbove;
  @override
  List<int> get subsurfacesAbove {
    if (_subsurfacesAbove is EqualUnmodifiableListView)
      return _subsurfacesAbove;
    // ignore: implicit_dynamic_type
    return EqualUnmodifiableListView(_subsurfacesAbove);
  }

  @override
  final Rect inputRegion;

  /// Create a copy of SurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @JsonKey(includeFromJson: false, includeToJson: false)
  @pragma('vm:prefer-inline')
  _$SurfaceStateCopyWith<_SurfaceState> get copyWith =>
      __$SurfaceStateCopyWithImpl<_SurfaceState>(this, _$identity);

  @override
  bool operator ==(Object other) {
    return identical(this, other) ||
        (other.runtimeType == runtimeType &&
            other is _SurfaceState &&
            (identical(other.role, role) || other.role == role) &&
            (identical(other.viewId, viewId) || other.viewId == viewId) &&
            (identical(other.textureId, textureId) ||
                other.textureId == textureId) &&
            (identical(other.surfacePosition, surfacePosition) ||
                other.surfacePosition == surfacePosition) &&
            (identical(other.surfaceSize, surfaceSize) ||
                other.surfaceSize == surfaceSize) &&
            (identical(other.scale, scale) || other.scale == scale) &&
            (identical(other.widgetKey, widgetKey) ||
                other.widgetKey == widgetKey) &&
            (identical(other.textureKey, textureKey) ||
                other.textureKey == textureKey) &&
            const DeepCollectionEquality()
                .equals(other._subsurfacesBelow, _subsurfacesBelow) &&
            const DeepCollectionEquality()
                .equals(other._subsurfacesAbove, _subsurfacesAbove) &&
            (identical(other.inputRegion, inputRegion) ||
                other.inputRegion == inputRegion));
  }

  @override
  int get hashCode => Object.hash(
      runtimeType,
      role,
      viewId,
      textureId,
      surfacePosition,
      surfaceSize,
      scale,
      widgetKey,
      textureKey,
      const DeepCollectionEquality().hash(_subsurfacesBelow),
      const DeepCollectionEquality().hash(_subsurfacesAbove),
      inputRegion);

  @override
  String toString() {
    return 'SurfaceState(role: $role, viewId: $viewId, textureId: $textureId, surfacePosition: $surfacePosition, surfaceSize: $surfaceSize, scale: $scale, widgetKey: $widgetKey, textureKey: $textureKey, subsurfacesBelow: $subsurfacesBelow, subsurfacesAbove: $subsurfacesAbove, inputRegion: $inputRegion)';
  }
}

/// @nodoc
abstract mixin class _$SurfaceStateCopyWith<$Res>
    implements $SurfaceStateCopyWith<$Res> {
  factory _$SurfaceStateCopyWith(
          _SurfaceState value, $Res Function(_SurfaceState) _then) =
      __$SurfaceStateCopyWithImpl;
  @override
  @useResult
  $Res call(
      {SurfaceRole role,
      int viewId,
      int textureId,
      Offset surfacePosition,
      Size surfaceSize,
      double scale,
      GlobalKey widgetKey,
      GlobalKey textureKey,
      List<int> subsurfacesBelow,
      List<int> subsurfacesAbove,
      Rect inputRegion});
}

/// @nodoc
class __$SurfaceStateCopyWithImpl<$Res>
    implements _$SurfaceStateCopyWith<$Res> {
  __$SurfaceStateCopyWithImpl(this._self, this._then);

  final _SurfaceState _self;
  final $Res Function(_SurfaceState) _then;

  /// Create a copy of SurfaceState
  /// with the given fields replaced by the non-null parameter values.
  @override
  @pragma('vm:prefer-inline')
  $Res call({
    Object? role = null,
    Object? viewId = null,
    Object? textureId = null,
    Object? surfacePosition = null,
    Object? surfaceSize = null,
    Object? scale = null,
    Object? widgetKey = null,
    Object? textureKey = null,
    Object? subsurfacesBelow = null,
    Object? subsurfacesAbove = null,
    Object? inputRegion = null,
  }) {
    return _then(_SurfaceState(
      role: null == role
          ? _self.role
          : role // ignore: cast_nullable_to_non_nullable
              as SurfaceRole,
      viewId: null == viewId
          ? _self.viewId
          : viewId // ignore: cast_nullable_to_non_nullable
              as int,
      textureId: null == textureId
          ? _self.textureId
          : textureId // ignore: cast_nullable_to_non_nullable
              as int,
      surfacePosition: null == surfacePosition
          ? _self.surfacePosition
          : surfacePosition // ignore: cast_nullable_to_non_nullable
              as Offset,
      surfaceSize: null == surfaceSize
          ? _self.surfaceSize
          : surfaceSize // ignore: cast_nullable_to_non_nullable
              as Size,
      scale: null == scale
          ? _self.scale
          : scale // ignore: cast_nullable_to_non_nullable
              as double,
      widgetKey: null == widgetKey
          ? _self.widgetKey
          : widgetKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      textureKey: null == textureKey
          ? _self.textureKey
          : textureKey // ignore: cast_nullable_to_non_nullable
              as GlobalKey,
      subsurfacesBelow: null == subsurfacesBelow
          ? _self._subsurfacesBelow
          : subsurfacesBelow // ignore: cast_nullable_to_non_nullable
              as List<int>,
      subsurfacesAbove: null == subsurfacesAbove
          ? _self._subsurfacesAbove
          : subsurfacesAbove // ignore: cast_nullable_to_non_nullable
              as List<int>,
      inputRegion: null == inputRegion
          ? _self.inputRegion
          : inputRegion // ignore: cast_nullable_to_non_nullable
              as Rect,
    ));
  }
}

// dart format on
