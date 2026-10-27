import 'dart:io';

bool _enabled(String name) {
  final value = Platform.environment[name];
  return value != null && value.isNotEmpty && value != '0';
}

final frameTimingTraceEnabled = _enabled('ZENITH_TRACE_FRAME_TIMING');
// Diagnostic A/B control. Normal rendering and theme defaults are unchanged.
final windowBlurDisabled = _enabled('ZENITH_DISABLE_WINDOW_BLUR');
