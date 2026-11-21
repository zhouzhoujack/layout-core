#include "utilsInternational.h"
#include "utilsString.h"

#if defined(HAVE_QT)
# include <QTextCodec>
#endif

#include <memory>
#include <iostream>
#include <locale.h>
#include <stdio.h>
#if !defined(_WIN32)
# include <langinfo.h>
#endif

namespace geo
{
// -----------------------------------------------------------------------------
//  International implementation


#if defined(HAVE_QT)

QTextCodec *ms_system_codec = 0;

QString to_qstring (const std::string &s)
{
  return QString::fromUtf8 (s.c_str ());
}

std::string to_string (const QString &s)
{
  return std::string (s.toUtf8 ().constData ());
}

#if !defined(_WIN32)
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
#endif

void initialize_codecs ()
{
  //  determine encoder for system strings
#ifdef _WIN32
  ms_system_codec = 0;
#elif defined(Q_WS_MAC)
  ms_system_codec = QTextCodec::codecForName ("UTF-8");
#else
  setlocale (LC_ALL, "");
  ms_system_codec = QTextCodec::codecForName (nl_langinfo (CODESET));
  if (! ms_system_codec) {
    ms_system_codec = QTextCodec::codecForName ("Latin-1");
  }
#endif

  static std::locale c_locale ("C");
  std::cout.imbue (c_locale);
  std::cin.imbue (c_locale);
  std::cerr.imbue (c_locale);
}

#else

std::string system_to_string (const std::string &s)
{
  return geo::to_string_from_local (s.c_str ());
}

std::string string_to_system (const std::string &s)
{
  return geo::to_local (s);
}

std::string tr (const char *s)
{
  //  TODO: this is a fallback implementation without translation
  return std::string (s);
}

#endif

}

#if ! defined(HAVE_QT)

std::string tr (const char *s)
{
  //  TODO: this is a fallback implementation without translation
  return std::string (s);
}

#endif
