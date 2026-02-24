import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:jdwm/src/core/controllers/window_stack_controller.dart';

part 'window_stack_provider.freezed.dart';

part 'window_stack_provider.g.dart';

final windowStackControllerProvider = Provider<WindowStackController>(
  (ref) => WindowStackController(),
);

@Riverpod(keepAlive: true)
class WindowStack extends _$WindowStack {
  @override
  WindowStackState build() {
    final controller = ref.read(windowStackControllerProvider);

    void syncState() {
      state = WindowStackState(stack: [...controller.stack]);
    }

    controller.addListener(syncState);
    ref.onDispose(() {
      controller.removeListener(syncState);
    });

    return WindowStackState(stack: [...controller.stack]);
  }

  void set(Iterable<int> list) {
    ref.read(windowStackControllerProvider).setAll(list);
  }

  void add(int viewId) {
    ref.read(windowStackControllerProvider).add(viewId);
  }

  void remove(int viewId) {
    ref.read(windowStackControllerProvider).remove(viewId);
  }

  void raise(int viewId) {
    ref.read(windowStackControllerProvider).raise(viewId);
  }

  void clear() {
    ref.read(windowStackControllerProvider).clear();
  }
}

@freezed
abstract class WindowStackState with _$WindowStackState {
  const WindowStackState._();

  const factory WindowStackState({
    required List<int> stack,
  }) = _WindowStackState;

  Iterable<int> get windows => stack;
}
