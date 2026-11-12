import 'dart:io';
import 'package:flutter/widgets.dart';

// Build as the main.dart of a standalone Flutter app with the same SDK version
// as the Sony engine. It deliberately has no desktop services or plugins.
Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Read immediately: a later native setenv cannot repair Dart's cached map.
  final environment = Platform.environment;
  final display = environment['DISPLAY'];
  final wayland = environment['WAYLAND_DISPLAY'];
  if (display == null ||
      display.isEmpty ||
      display == environment['JDWM_TEST_HOST_DISPLAY'] ||
      wayland == null ||
      wayland.isEmpty ||
      environment['XDG_SESSION_TYPE'] != 'wayland') {
    throw StateError('Missing compositor display environment at Dart startup');
  }
  // Supplying an override exercises Dart's parent-environment merge, as used by
  // the desktop launcher. Inspect only the relevant values; do not log env.
  final child = await Process.run('/usr/bin/env', const [],
      environment: {'JDWM_ENVIRONMENT_PROBE': '1'});
  final lines = (child.stdout as String).split('\n');
  if (child.exitCode != 0 ||
      !lines.contains('DISPLAY=$display') ||
      !lines.contains('WAYLAND_DISPLAY=$wayland') ||
      !lines.contains('XDG_SESSION_TYPE=wayland')) {
    throw StateError('Child process did not inherit compositor displays');
  }
  debugPrint(
      'session-environment: inherited DISPLAY=$display WAYLAND_DISPLAY=$wayland');
  runApp(const ShutdownAnimation());
}

class ShutdownAnimation extends StatefulWidget {
  const ShutdownAnimation({super.key});

  @override
  State<ShutdownAnimation> createState() => _ShutdownAnimationState();
}

class _ShutdownAnimationState extends State<ShutdownAnimation>
    with SingleTickerProviderStateMixin {
  late final controller = AnimationController(
    vsync: this,
    duration: const Duration(seconds: 1),
  )..repeat();

  @override
  Widget build(BuildContext context) => AnimatedBuilder(
        animation: controller,
        builder: (_, __) => ColoredBox(
          color: Color.lerp(
            const Color(0xff114477),
            const Color(0xff99aabb),
            controller.value,
          )!,
        ),
      );

  @override
  void dispose() {
    controller.dispose();
    super.dispose();
  }
}
