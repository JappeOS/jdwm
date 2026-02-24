enum ToplevelDecoration {
  none,
  clientSide,
  serverSide;

  static ToplevelDecoration fromInt(int n) {
    switch (n) {
      case 0:
        return none;
      case 1:
        return clientSide;
      case 2:
        return serverSide;
      default:
        return none;
    }
  }
}
