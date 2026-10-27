import 'package:flutter/foundation.dart';
import 'package:flutter/scheduler.dart';
import 'dart:ui' show Rect;

import 'frame_trace_environment.dart'
    if (dart.library.io) 'frame_trace_environment_io.dart';

export 'frame_trace_environment.dart'
    if (dart.library.io) 'frame_trace_environment_io.dart'
    show windowBlurDisabled;

/// Opt-in summaries of Flutter build/raster timings and backdrop paints.
/// Raster duration measures CPU submission, not asynchronous GPU completion.
class FlutterFrameTrace {
  static int _owners = 0;
  static final _clock = Stopwatch();
  static int _frames = 0, _buildUs = 0, _rasterUs = 0;
  static int _maxBuildUs = 0, _maxRasterUs = 0;
  static int _backdrops = 0, _optimized = 0;
  static double _area = 0, _filteredArea = 0;
  static int _windows = 0, _culled = 0;
  static double _windowArea = 0, _visibleArea = 0;

  static void occlusion(Rect bounds, List<Rect> visible) {
    if (!frameTimingTraceEnabled || _owners == 0) return;
    _windows++;
    if (visible.isEmpty) _culled++;
    if (!bounds.isEmpty) _windowArea += bounds.width * bounds.height;
    for (final rect in visible) {
      _visibleArea += rect.width * rect.height;
    }
  }

  static void attach() {
    if (!frameTimingTraceEnabled || _owners++ != 0) return;
    _clock.start();
    SchedulerBinding.instance.addTimingsCallback(_timings);
    debugPrint(
        'zenith:flutter-trace enabled window_blur_disabled=$windowBlurDisabled');
  }

  static void detach() {
    if (!frameTimingTraceEnabled || _owners == 0 || --_owners != 0) return;
    SchedulerBinding.instance.removeTimingsCallback(_timings);
    _clock.stop();
    _reset();
  }

  static void backdrop(double area, double filteredArea) {
    if (!frameTimingTraceEnabled || _owners == 0) return;
    _backdrops++;
    if (filteredArea < area) _optimized++;
    _area += area;
    _filteredArea += filteredArea;
  }

  static void _timings(List<FrameTiming> timings) {
    for (final timing in timings) {
      final build = timing.buildDuration.inMicroseconds;
      final raster = timing.rasterDuration.inMicroseconds;
      _frames++;
      _buildUs += build;
      _rasterUs += raster;
      if (build > _maxBuildUs) _maxBuildUs = build;
      if (raster > _maxRasterUs) _maxRasterUs = raster;
    }
    if (_clock.elapsedMilliseconds < 1000 || _frames == 0) return;
    debugPrint('zenith:flutter-timing frames=$_frames '
        'build_avg_ms=${(_buildUs / _frames / 1000).toStringAsFixed(3)} '
        'build_max_ms=${(_maxBuildUs / 1000).toStringAsFixed(3)} '
        'raster_avg_ms=${(_rasterUs / _frames / 1000).toStringAsFixed(3)} '
        'raster_max_ms=${(_maxRasterUs / 1000).toStringAsFixed(3)} '
        'backdrop_paints=$_backdrops backdrop_optimized=$_optimized '
        'backdrop_area=${_area.round()} filtered_area=${_filteredArea.round()} '
        'occlusion_windows=$_windows occlusion_culled=$_culled '
        'window_area=${_windowArea.round()} visible_area=${_visibleArea.round()}');
    _reset();
  }

  static void _reset() {
    _frames = _buildUs = _rasterUs = _maxBuildUs = _maxRasterUs = 0;
    _backdrops = _optimized = 0;
    _area = _filteredArea = 0;
    _windows = _culled = 0;
    _windowArea = _visibleArea = 0;
    _clock.reset();
  }
}
