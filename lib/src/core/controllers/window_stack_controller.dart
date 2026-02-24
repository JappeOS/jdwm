import 'package:flutter/foundation.dart';

class WindowStackController extends ChangeNotifier {
  final List<int> _stack = <int>[];

  List<int> get stack => List<int>.unmodifiable(_stack);

  void setAll(Iterable<int> list) {
    _stack
      ..clear()
      ..addAll(list);
    notifyListeners();
  }

  void add(int viewId) {
    _stack.add(viewId);
    notifyListeners();
  }

  void remove(int viewId) {
    _stack.removeWhere((id) => id == viewId);
    notifyListeners();
  }

  void raise(int viewId) {
    remove(viewId);
    add(viewId);
  }

  void clear() {
    _stack.clear();
    notifyListeners();
  }
}
