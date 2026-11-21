#ifndef HDR_utilsExceptions
#define HDR_utilsExceptions

#include "utilsCommon.h"

namespace geo
{
  class Exception;
}

namespace std
{
  class exception;
}

class QWidget;

namespace geo
{

void TL_PUBLIC handle_exception_silent (const geo::Exception &);
void TL_PUBLIC handle_exception_silent (const std::exception &);
void TL_PUBLIC handle_exception_silent ();
void TL_PUBLIC handle_exception_ui (const geo::Exception &, QWidget *parent = 0);
void TL_PUBLIC handle_exception_ui (const std::exception &, QWidget *parent = 0);
void TL_PUBLIC handle_exception_ui (QWidget *parent = 0);
void TL_PUBLIC handle_exception (const geo::Exception &);
void TL_PUBLIC handle_exception (const std::exception &);
void TL_PUBLIC handle_exception ();

/**
 *  @brief Installs the GUI exception handlers
 */
void TL_PUBLIC set_ui_exception_handlers (void (*handler_tl) (const geo::Exception &, QWidget *parent),
                                            void (*handler_std) (const std::exception &, QWidget *parent),
                                            void (*handler_default) (QWidget *parent));

//  The BEGIN_PROTECTED .. END_PROTECTED macros are supposed to enclose a protected block
//  The main purpose is to provide a unified method of handling exceptions

#define BEGIN_PROTECTED_SILENT try {

#define END_PROTECTED_SILENT \
} catch (geo::Exception &ex) { \
  geo::handle_exception_silent (ex); \
} catch (std::exception &ex) { \
  geo::handle_exception_silent (ex); \
} catch (...) { \
  geo::handle_exception_silent (); \
} 

#define BEGIN_PROTECTED try {

#define END_PROTECTED \
} catch (geo::Exception &ex) { \
  geo::handle_exception (ex); \
} catch (std::exception &ex) { \
  geo::handle_exception (ex); \
} catch (...) { \
  geo::handle_exception (); \
} 

#define END_PROTECTED_W(w) \
} catch (geo::Exception &ex) { \
  geo::handle_exception_ui (ex, w); \
} catch (std::exception &ex) { \
  geo::handle_exception_ui (ex, w); \
} catch (...) { \
  geo::handle_exception_ui (w); \
} 

#define BEGIN_PROTECTED_CLEANUP bool __error_caught = false; try {

#define END_PROTECTED_CLEANUP_W(w) \
} catch (geo::Exception &ex) { \
  __error_caught = true; \
  geo::handle_exception_ui (ex, w); \
} catch (std::exception &ex) { \
  __error_caught = true; \
  geo::handle_exception_ui (ex, w); \
} catch (...) { \
  __error_caught = true; \
  geo::handle_exception_ui (w); \
} \
if (__error_caught)

#define END_PROTECTED_CLEANUP \
} catch (geo::Exception &ex) { \
  __error_caught = true; \
  geo::handle_exception (ex); \
} catch (std::exception &ex) { \
  __error_caught = true; \
  geo::handle_exception (ex); \
} catch (...) { \
  __error_caught = true; \
  geo::handle_exception (); \
} \
if (__error_caught)

}

#endif

