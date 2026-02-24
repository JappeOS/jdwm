import 'package:flutter/services.dart';
import 'package:shadcn_flutter/shadcn_flutter.dart';

sealed class ClientCursor {
  static Image get(SystemMouseCursor cursor) {
    String? asset;
    switch (cursor) {
      case SystemMouseCursors.grabbing:
        asset = 'assets/cursor/grabbing.png';
        break;
      case SystemMouseCursors.resizeUpDown:
        asset = 'assets/cursor/sb_v_double_arrow.png';
        break;
      case SystemMouseCursors.resizeLeftRight:
        asset = 'assets/cursor/sb_h_double_arrow.png';
        break;
      case SystemMouseCursors.resizeUpLeftDownRight:
        asset = 'assets/cursor/bd_double_arrow.png';
        break;
      case SystemMouseCursors.resizeUpRightDownLeft:
        asset = 'assets/cursor/fd_double_arrow.png';
        break;
      default:
        asset = null;
    }

    if (asset == null) {
      return Image.memory(Uint8List(0));
    }

    return Image.asset(
      asset,
      package: 'jdwm',
      width: 24,
      height: 24,
    );
  }
}