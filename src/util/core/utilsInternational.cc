#include "utilsInternational.h"
#include "utilsString.h"

# include <QTextCodec>

#include <memory>
#include <iostream>
#include <locale.h>
#include <stdio.h>
#include <langinfo.h>

namespace geo
{
// -----------------------------------------------------------------------------
//  International implementation



QTextCodec *ms_system_codec = 0;

QString to_qstring (const std::string &s)
{
  return QString::fromUtf8 (s.c_str ());
}

std::string to_string (const QString &s)
{
  return std::string (s.toUtf8 ().constData ());
}

std::string system_to_string (const std::string &s)
{
  if (! ms_system_codec) {
    initialize_codecs ();
  }

  QString qs (ms_system_codec->toUnicode (s.c_str ()));
  return std::string (qs.toUtf8 ().constData ());
}

std::string string_to_system (const std::string &s)
{
  if (! ms_system_codec) {
    initialize_codecs ();
  }

  QString qs = QString::fromUtf8 (s.c_str ());
  return std::string (ms_system_codec->fromUnicode (qs).constData ());
}

void initialize_codecs ()
{
  //  determine encoder for system strings
  setlocale (LC_ALL, "");
  ms_system_codec = QTextCodec::codecForName (nl_langinfo (CODESET));
  if (! ms_system_codec) {
    ms_system_codec = QTextCodec::codecForName ("Latin-1");
  }

  static std::locale c_locale ("C");
  std::cout.imbue (c_locale);
  std::cin.imbue (c_locale);
  std::cerr.imbue (c_locale);
}


}

