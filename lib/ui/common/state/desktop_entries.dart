import 'dart:io';

import 'package:freedesktop_desktop_entry/freedesktop_desktop_entry.dart';
import 'package:jovial_svg/jovial_svg.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

part 'desktop_entries.g.dart';

@Riverpod(keepAlive: true)
Future<Map<String, DesktopEntry>> installedDesktopEntries(Ref ref) async {
  return parseAllInstalledDesktopFiles();
}

@Riverpod(keepAlive: true)
Future<Map<String, LocalizedDesktopEntry>> localizedDesktopEntries(Ref ref) async {
  final desktopEntries = await ref.watch(installedDesktopEntriesProvider.future);
  return desktopEntries.map((key, value) => MapEntry(key, value.localize(lang: 'en')));
}

@Riverpod(keepAlive: true)
Future<Iterable<LocalizedDesktopEntry>> appDrawerDesktopEntries(Ref ref) async {
  final localizedDesktopEntries = await ref.watch(localizedDesktopEntriesProvider.future);
  return localizedDesktopEntries.values.where((element) => !element.desktopEntry.isHidden());
}

@Riverpod(keepAlive: true)
Future<FreedesktopIconThemes> iconThemes(Ref ref) async {
  final themes = FreedesktopIconThemes();
  await themes.loadThemes();
  return themes;
}

@Riverpod(keepAlive: true)
Future<File?> icon(Ref ref, IconQuery query) async {
  final themes = await ref.watch(iconThemesProvider.future);
  return themes.findIcon(query);
}

@Riverpod(keepAlive: true)
Future<ScalableImage> fileToScalableImage(Ref ref, String path) async {
  String svg = await File(path).readAsString();
  return ScalableImage.fromSvgString(svg);
}
