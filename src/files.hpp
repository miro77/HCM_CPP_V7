// Dateizugriff mit Fallback ohne Gross-/Kleinschreibung: Die Schnittstelle
// wandelt Pfade in Grossbuchstaben (LH-FA-01); auf Dateisystemen mit
// Gross-/Kleinschreibung wird jede Pfadkomponente unabhaengig davon gesucht.
#pragma once

#include <fstream>
#include <string>

namespace hcmms {

// Oeffnet `path` binaer zum Lesen; liefert false, wenn die Datei fehlt.
bool open_binary(std::ifstream &f, const std::string &path);

// Liest Record `rec` (1-basiert) der Laenge `len` aus einer Datei mit festen
// Recordlaengen. Liefert false, wenn der Record nicht vollstaendig existiert.
bool read_record(std::ifstream &f, long rec, std::size_t len, unsigned char *buf);

}  // namespace hcmms
