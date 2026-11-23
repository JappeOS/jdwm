import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

part 'mouse_button_tracker.g.dart';

@Riverpod(keepAlive: true)
MouseButtonTracker mouseButtonTracker(Ref ref) => MouseButtonTracker();

/// Converts Flutter's pressed-button mask into individual Wayland transitions.
class MouseButtonTracker {
  int _buttons = 0;

  List<MouseButtonEvent> trackButtonChanges(int buttons) {
    var changed = _buttons ^ buttons;
    if (changed == 0) return const [];

    _buttons = buttons;
    final events = <MouseButtonEvent>[];
    while (changed != 0) {
      final button = changed & -changed; // Lowest changed bit.
      final pressed = (buttons & button) != 0;
      events.add(MouseButtonEvent(button,
          pressed ? MouseButtonState.pressed : MouseButtonState.released));
      changed ^= button;
    }
    
    return events;
  }
}

enum MouseButtonState {
  pressed,
  released,
}

class MouseButtonEvent {
  int button;
  MouseButtonState state;

  MouseButtonEvent(this.button, this.state);

  @override
  String toString() {
    return 'MouseButtonEvent{button: $button, state: $state}';
  }
}
