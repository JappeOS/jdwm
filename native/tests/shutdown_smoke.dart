import 'package:flutter/widgets.dart';

// Build as the main.dart of a standalone Flutter app with the same SDK version
// as the Sony engine. It deliberately has no desktop services or plugins.
void main() => runApp(const ShutdownAnimation());

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
