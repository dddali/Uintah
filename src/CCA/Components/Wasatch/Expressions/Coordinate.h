#ifndef Coordinates_h
#define Coordinates_h

/*
 * The MIT License
 *
 * Copyright (c) 2012 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

//-- ExprLib Includes --//
#include <expression/Expression.h>

//-- Wasatch Includes --//
#include <CCA/Components/Wasatch/FieldTypes.h>
#include <CCA/Components/Wasatch/Operators/Operators.h>
#include <CCA/Components/Wasatch/Operators/OperatorTypes.h>

namespace Wasatch{
  
  /**
   *  \class 	 Coordinates
   *  \ingroup   Expressions
   *  \ingroup	 WasatchCore
   *  \author 	 Tony Saad, James C. Sutherland
   *  \date 	 November, 2013
   *
   *  \brief Expression to compute coordinates.
   *
   *  NOTE: this expression BREAKS WITH CONVENTION!  Notably, it has
   *  uintah tenticles that reach into it, and mixes SpatialOps and
   *  Uintah constructs.
   *
   *  This expression does play well with expression graphs, however.
   *  There are only a few places where Uintah reaches in.
   *
   *  Because of the hackery going on here, this expression is placed in
   *  the Wasatch namespace.  This should reinforce the concept that it
   *  is not intended for external use.
   */
  template< typename FieldT>
  class Coordinates
  : public Expr::Expression<FieldT>
  {
    const Uintah::Patch* patch_;
    int idir_; // x = 0, y = 1, z = 2
    SCIRun::Vector shift_; // shift spacing by -dx/2 for staggered fields
    Coordinates(const int idir);
    
  public:
    
    class Builder : public Expr::ExpressionBuilder
    {
    private:
      int idir_;
    public:
      Builder( const Expr::Tag& result );
      ~Builder(){}
      Expr::ExpressionBase* build() const;
    };
    
    ~Coordinates();
    
    
    /**
     *  \brief Save pointer to the patch associated with this expression. This
     *          is needed to set boundary conditions and extract other mesh info.
     */
    void set_patch( const Uintah::Patch* const patch );
    
    void advertise_dependents( Expr::ExprDeps& exprDeps );
    void bind_fields( const Expr::FieldManagerList& fml );
    void evaluate();
    
  };
} // namespace Wasatch

#endif // Coordinates_h
