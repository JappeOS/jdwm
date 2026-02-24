import 'dart:ffi' as ffi;

/// Placeholder for future FFI bindings.
///
/// When the native backend becomes a shared library, this will load it and
/// expose the native API to Dart/Flutter.
class ZenithBackendFfi {
  ZenithBackendFfi(this._lib);

  final ffi.DynamicLibrary _lib;

  // TODO: bind zenith_backend_run and other APIs here.
}
