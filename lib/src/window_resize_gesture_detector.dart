import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class WindowResizeGestureDetector extends StatelessWidget {
  final double borderThickness;
  final Map<Alignment, GestureDragUpdateCallback> listeners;
  final void Function(SystemMouseCursor, DragUpdateDetails) dragWithCursor;
  final GestureDragEndCallback onPanEnd;

  const WindowResizeGestureDetector({
    super.key,
    required this.borderThickness,
    required this.listeners,
    required this.dragWithCursor,
    required this.onPanEnd,
  });

  @override
  Widget build(BuildContext context) {
    final cornerSize = borderThickness * 2;
    return Stack(
      children: [
        Positioned(
          left: borderThickness,
          right: borderThickness,
          top: 0,
          height: borderThickness,
          child: buildGestureDetector(
            listeners[Alignment.topCenter]!,
            SystemMouseCursors.resizeUpDown,
          ),
        ),
        Positioned(
          left: borderThickness,
          right: borderThickness,
          bottom: 0,
          height: borderThickness,
          child: buildGestureDetector(
            listeners[Alignment.bottomCenter]!,
            SystemMouseCursors.resizeUpDown,
          ),
        ),
        Positioned(
          left: 0,
          top: borderThickness,
          bottom: borderThickness,
          width: borderThickness,
          child: buildGestureDetector(
            listeners[Alignment.centerLeft]!,
            SystemMouseCursors.resizeLeftRight,
          ),
        ),
        Positioned(
          right: 0,
          top: borderThickness,
          bottom: borderThickness,
          width: borderThickness,
          child: buildGestureDetector(
            listeners[Alignment.centerRight]!,
            SystemMouseCursors.resizeLeftRight,
          ),
        ),
        Positioned(
          left: 0,
          top: 0,
          width: cornerSize,
          height: cornerSize,
          child: buildGestureDetector(
            listeners[Alignment.topLeft]!,
            SystemMouseCursors.resizeUpLeftDownRight,
          ),
        ),
        Positioned(
          right: 0,
          top: 0,
          width: cornerSize,
          height: cornerSize,
          child: buildGestureDetector(
            listeners[Alignment.topRight]!,
            SystemMouseCursors.resizeUpRightDownLeft,
          ),
        ),
        Positioned(
          left: 0,
          bottom: 0,
          width: cornerSize,
          height: cornerSize,
          child: buildGestureDetector(
            listeners[Alignment.bottomLeft]!,
            SystemMouseCursors.resizeUpRightDownLeft,
          ),
        ),
        Positioned(
          right: 0,
          bottom: 0,
          width: cornerSize,
          height: cornerSize,
          child: buildGestureDetector(
            listeners[Alignment.bottomRight]!,
            SystemMouseCursors.resizeUpLeftDownRight,
          ),
        ),
      ],
    );
  }

  Widget buildGestureDetector(
    GestureDragUpdateCallback onPanUpdate,
    SystemMouseCursor cursor,
  ) {
    return MouseRegion(
      cursor: cursor,
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onPanUpdate: (details) {
          dragWithCursor(cursor, details);
          onPanUpdate(details);
        },
        onPanEnd: onPanEnd,
      ),
    );
  }
}
