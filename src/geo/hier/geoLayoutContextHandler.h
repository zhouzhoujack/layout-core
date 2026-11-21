#ifndef HDR_geoLayoutContextHandler
#define HDR_geoLayoutContextHandler

#include "utilsExpression.h"
#include "geoLayout.h"

namespace geo
{

/**
 * @brief  A layout providing a context for the expression evaluation
 */
class LayoutContextHandler
  : public geo::ContextHandler
{
public:
  /**
   *  @brief Constructor: provides a context from a layout
   *  This object will provide bindings for <..> (layers) and <<..>> (cells) expressions.
   *  Plus it will provide a database unit value.
   */
  LayoutContextHandler (const geo::Layout *layout);

  /**
   *  @brief The constructor with a non-const layout context
   *  In the non-const context, layers or cells will be created if they
   *  are not there yet. To enable non-const context, set the "can_modify"
   *  flag to true.
   */
  LayoutContextHandler (geo::Layout *layout, bool can_modify);

  /**
   *  @brief Provide <..> bindings.
   */
  virtual geo::Variant eval_bracket (const std::string &content) const;

  /**
   *  @brief Provide <<..>> bindings.
   */
  virtual geo::Variant eval_double_bracket (const std::string &content) const;

  /**
   *  @brief Provide a database unit value
   */
  virtual double dbu () const
  {
    return mp_layout->dbu ();
  }

private:
  const geo::Layout *mp_layout;
  geo::Layout *mp_layout_nc;
};

}

#endif

