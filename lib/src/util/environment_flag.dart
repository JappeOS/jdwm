bool environmentFlagEnabled(String? value) =>
    value != null &&
    value.isNotEmpty &&
    !const {'0', 'false', 'off', 'no'}.contains(value.toLowerCase());
