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
    return Column(
      children: [
        Row(
          children: [
            buildGestureDetector(
              borderThickness,
              borderThickness,
              listeners[Alignment.topLeft]!,
              SystemMouseCursors.resizeUpLeftDownRight,
            ),
            Expanded(
              child: buildGestureDetector(
                null,
                borderThickness,
                listeners[Alignment.topCenter]!,
                SystemMouseCursors.resizeUpDown,
              ),
            ),
            buildGestureDetector(
              borderThickness,
              borderThickness,
              listeners[Alignment.topRight]!,
              SystemMouseCursors.resizeUpRightDownLeft,
            ),
          ],
        ),
        Expanded(
          child: Row(
            children: [
              buildGestureDetector(
                borderThickness,
                null,
                listeners[Alignment.centerLeft]!,
                SystemMouseCursors.resizeLeftRight,
              ),
              const Spacer(),
              buildGestureDetector(
                borderThickness,
                null,
                listeners[Alignment.centerRight]!,
                SystemMouseCursors.resizeLeftRight,
              ),
            ],
          ),
        ),
        Row(
          children: [
            buildGestureDetector(
              borderThickness,
              borderThickness,
              listeners[Alignment.bottomLeft]!,
              SystemMouseCursors.resizeUpRightDownLeft,
            ),
            Expanded(
              child: buildGestureDetector(
                null,
                borderThickness,
                listeners[Alignment.bottomCenter]!,
                SystemMouseCursors.resizeUpDown,
              ),
            ),
            buildGestureDetector(
              borderThickness,
              borderThickness,
              listeners[Alignment.bottomRight]!,
              SystemMouseCursors.resizeUpLeftDownRight,
            ),
          ],
        ),
      ],
    );
  }

  Widget buildGestureDetector(
    double? width,
    double? height,
    GestureDragUpdateCallback onPanUpdate,
    SystemMouseCursor cursor,
  ) {
    return SizedBox(
      width: width,
      height: height,
      child: MouseRegion(
        cursor: cursor,
        child: GestureDetector(
          onPanUpdate: (details) {
            dragWithCursor(cursor, details);
            onPanUpdate(details);
          },
          onPanEnd: onPanEnd,
        ),
      ),
    );
  }
}
