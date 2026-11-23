import 'dart:io';

import 'environment_flag.dart';

bool _enabled(String name) => environmentFlagEnabled(Platform.environment[name]);

final frameTimingTraceEnabled = _enabled('ZENITH_TRACE_FRAME_TIMING');
// Diagnostic A/B control. Normal rendering and theme defaults are unchanged.
final windowBlurDisabled = _enabled('ZENITH_DISABLE_WINDOW_BLUR');
