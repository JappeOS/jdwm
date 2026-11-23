/// A Wayland surface can disappear while Flutter is still painting its last
/// frame. Release its texture after both native destruction and final detach.
class SurfaceTextureLifetime {
  SurfaceTextureLifetime(this._unregister);

  final void Function(int) _unregister;
  final Map<int, int> _users = {};
  final Set<int> _destroyed = {};

  void attach(int id) => _users.update(id, (n) => n + 1, ifAbsent: () => 1);

  void detach(int id) {
    final count = _users[id]!;
    if (count > 1) {
      _users[id] = count - 1;
      return;
    }
    _users.remove(id);
    if (_destroyed.remove(id)) _unregister(id);
  }

  void destroy(int id) {
    if (_users.containsKey(id)) {
      _destroyed.add(id);
    } else {
      _unregister(id);
    }
  }
}
