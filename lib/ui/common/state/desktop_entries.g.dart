// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'desktop_entries.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(installedDesktopEntries)
final installedDesktopEntriesProvider = InstalledDesktopEntriesProvider._();

final class InstalledDesktopEntriesProvider extends $FunctionalProvider<
        AsyncValue<Map<String, DesktopEntry>>,
        Map<String, DesktopEntry>,
        FutureOr<Map<String, DesktopEntry>>>
    with
        $FutureModifier<Map<String, DesktopEntry>>,
        $FutureProvider<Map<String, DesktopEntry>> {
  InstalledDesktopEntriesProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'installedDesktopEntriesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$installedDesktopEntriesHash();

  @$internal
  @override
  $FutureProviderElement<Map<String, DesktopEntry>> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<Map<String, DesktopEntry>> create(Ref ref) {
    return installedDesktopEntries(ref);
  }
}

String _$installedDesktopEntriesHash() =>
    r'a6612ad27d0eee11b45871177f1cec487f5f7ca0';

@ProviderFor(localizedDesktopEntries)
final localizedDesktopEntriesProvider = LocalizedDesktopEntriesProvider._();

final class LocalizedDesktopEntriesProvider extends $FunctionalProvider<
        AsyncValue<Map<String, LocalizedDesktopEntry>>,
        Map<String, LocalizedDesktopEntry>,
        FutureOr<Map<String, LocalizedDesktopEntry>>>
    with
        $FutureModifier<Map<String, LocalizedDesktopEntry>>,
        $FutureProvider<Map<String, LocalizedDesktopEntry>> {
  LocalizedDesktopEntriesProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'localizedDesktopEntriesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$localizedDesktopEntriesHash();

  @$internal
  @override
  $FutureProviderElement<Map<String, LocalizedDesktopEntry>> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<Map<String, LocalizedDesktopEntry>> create(Ref ref) {
    return localizedDesktopEntries(ref);
  }
}

String _$localizedDesktopEntriesHash() =>
    r'ba4962cf07ecbcc73ee6b9c47716e8306cec42cc';

@ProviderFor(appDrawerDesktopEntries)
final appDrawerDesktopEntriesProvider = AppDrawerDesktopEntriesProvider._();

final class AppDrawerDesktopEntriesProvider extends $FunctionalProvider<
        AsyncValue<Iterable<LocalizedDesktopEntry>>,
        Iterable<LocalizedDesktopEntry>,
        FutureOr<Iterable<LocalizedDesktopEntry>>>
    with
        $FutureModifier<Iterable<LocalizedDesktopEntry>>,
        $FutureProvider<Iterable<LocalizedDesktopEntry>> {
  AppDrawerDesktopEntriesProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'appDrawerDesktopEntriesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$appDrawerDesktopEntriesHash();

  @$internal
  @override
  $FutureProviderElement<Iterable<LocalizedDesktopEntry>> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<Iterable<LocalizedDesktopEntry>> create(Ref ref) {
    return appDrawerDesktopEntries(ref);
  }
}

String _$appDrawerDesktopEntriesHash() =>
    r'b6dd74d1d98df2497bda124ec44c4e9cf5d2bc25';

@ProviderFor(iconThemes)
final iconThemesProvider = IconThemesProvider._();

final class IconThemesProvider extends $FunctionalProvider<
        AsyncValue<FreedesktopIconThemes>,
        FreedesktopIconThemes,
        FutureOr<FreedesktopIconThemes>>
    with
        $FutureModifier<FreedesktopIconThemes>,
        $FutureProvider<FreedesktopIconThemes> {
  IconThemesProvider._()
      : super(
          from: null,
          argument: null,
          retry: null,
          name: r'iconThemesProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$iconThemesHash();

  @$internal
  @override
  $FutureProviderElement<FreedesktopIconThemes> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<FreedesktopIconThemes> create(Ref ref) {
    return iconThemes(ref);
  }
}

String _$iconThemesHash() => r'1c7c3c0be2ff136c8bf47ebb5c742cfe1d818a5a';

@ProviderFor(icon)
final iconProvider = IconFamily._();

final class IconProvider
    extends $FunctionalProvider<AsyncValue<File?>, File?, FutureOr<File?>>
    with $FutureModifier<File?>, $FutureProvider<File?> {
  IconProvider._(
      {required IconFamily super.from, required IconQuery super.argument})
      : super(
          retry: null,
          name: r'iconProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$iconHash();

  @override
  String toString() {
    return r'iconProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $FutureProviderElement<File?> $createElement($ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<File?> create(Ref ref) {
    final argument = this.argument as IconQuery;
    return icon(
      ref,
      argument,
    );
  }

  @override
  bool operator ==(Object other) {
    return other is IconProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$iconHash() => r'605ed7e9f5a72215754288279c3350cd183d44fb';

final class IconFamily extends $Family
    with $FunctionalFamilyOverride<FutureOr<File?>, IconQuery> {
  IconFamily._()
      : super(
          retry: null,
          name: r'iconProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  IconProvider call(
    IconQuery query,
  ) =>
      IconProvider._(argument: query, from: this);

  @override
  String toString() => r'iconProvider';
}

@ProviderFor(fileToScalableImage)
final fileToScalableImageProvider = FileToScalableImageFamily._();

final class FileToScalableImageProvider extends $FunctionalProvider<
        AsyncValue<ScalableImage>, ScalableImage, FutureOr<ScalableImage>>
    with $FutureModifier<ScalableImage>, $FutureProvider<ScalableImage> {
  FileToScalableImageProvider._(
      {required FileToScalableImageFamily super.from,
      required String super.argument})
      : super(
          retry: null,
          name: r'fileToScalableImageProvider',
          isAutoDispose: false,
          dependencies: null,
          $allTransitiveDependencies: null,
        );

  @override
  String debugGetCreateSourceHash() => _$fileToScalableImageHash();

  @override
  String toString() {
    return r'fileToScalableImageProvider'
        ''
        '($argument)';
  }

  @$internal
  @override
  $FutureProviderElement<ScalableImage> $createElement(
          $ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<ScalableImage> create(Ref ref) {
    final argument = this.argument as String;
    return fileToScalableImage(
      ref,
      argument,
    );
  }

  @override
  bool operator ==(Object other) {
    return other is FileToScalableImageProvider && other.argument == argument;
  }

  @override
  int get hashCode {
    return argument.hashCode;
  }
}

String _$fileToScalableImageHash() =>
    r'3e3959d33bc338638cbd82e5fdbe8f0d0a4d2dfd';

final class FileToScalableImageFamily extends $Family
    with $FunctionalFamilyOverride<FutureOr<ScalableImage>, String> {
  FileToScalableImageFamily._()
      : super(
          retry: null,
          name: r'fileToScalableImageProvider',
          dependencies: null,
          $allTransitiveDependencies: null,
          isAutoDispose: false,
        );

  FileToScalableImageProvider call(
    String path,
  ) =>
      FileToScalableImageProvider._(argument: path, from: this);

  @override
  String toString() => r'fileToScalableImageProvider';
}
