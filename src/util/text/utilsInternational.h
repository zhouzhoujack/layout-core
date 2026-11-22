#ifndef HDR_utilsInternational
#define HDR_utilsInternational

#include "utilsCommon.h"

#include <string>

# include <QString>
//  provides QObject for tr
# include <QObject>

/**
 *  @brief Generic tr function for non-Qt and Qt builds
 */
inline QString tr (const char *s)
{
  return QObject::tr (s);
}

namespace geo
{

/**
 *  @brief Convert a UTF8 std::string to a QString
 */
TL_PUBLIC QString to_qstring (const std::string &s);

/**
 *  @brief Convert a QString to a UTF8 std::string
 */
TL_PUBLIC std::string to_string (const QString &s);

/**
 *  @brief Dummy conversion for convenience and as non-Qt replacement for to_string (const QString &)
 */
inline const std::string &to_string (const std::string &s)
{
  return s;
}

/**
 *  @brief Convert a system encoding std::string to a UTF8 std::string
 */
TL_PUBLIC std::string system_to_string (const std::string &s);

/**
 *  @brief Convert a UTF8 string to a system encoding string
 */
TL_PUBLIC std::string string_to_system (const std::string &s);

/**
 *  @brief Initialize the codecs
 */
TL_PUBLIC void initialize_codecs ();

}

#endif

