
/*
 *  DenseMatrix.h:  Dense matrices
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#ifndef SCI_project_DenseMatrix_h
#define SCI_project_DenseMatrix_h 1

#include <SCICore/share/share.h>

#include <SCICore/Datatypes/Matrix.h>
#include <SCICore/Math/MiscMath.h>

namespace SCICore {
namespace Datatypes {

using SCICore::Math::Abs;
using namespace SCICore::Datatypes;

class SCICORESHARE DenseMatrix : public Matrix {
    int nc;
    int nr;
    double minVal;
    double maxVal;
    double** data;
    double* dataptr;
public:
    DenseMatrix();
    DenseMatrix(int, int);
    virtual ~DenseMatrix();
    DenseMatrix(const DenseMatrix&);
    DenseMatrix& operator=(const DenseMatrix&);
    virtual double& get(int, int);
    virtual void put(int, int, const double&);
    virtual int nrows() const;
    virtual int ncols() const;
    virtual double minValue();
    virtual double maxValue();
    inline double* getData() { return dataptr;}
    virtual void getRowNonzeros(int r, Array1<int>& idx, Array1<double>& val);
    virtual void solve(ColumnMatrix&);
    virtual void zero();

    virtual void mult(const ColumnMatrix& x, ColumnMatrix& b,
		      int& flops, int& memrefs, int beg=-1, int end=-1) const;
    virtual void mult_transpose(const ColumnMatrix& x, ColumnMatrix& b,
				int& flops, int& memrefs,
				int beg=-1, int end=-1);
    virtual void print();

    MatrixRow operator[](int r);
    // Persistent representation...
    virtual void io(Piostream&);
    static PersistentTypeID type_id;

    void invert();
    void mult(double s);

    friend SCICORESHARE void Mult(DenseMatrix&, const DenseMatrix&, const DenseMatrix&);
    friend SCICORESHARE void Add(DenseMatrix&, const DenseMatrix&, const DenseMatrix&);
    friend SCICORESHARE void Mult_trans_X(DenseMatrix&, const DenseMatrix&, const DenseMatrix&);
    friend SCICORESHARE void Mult_X_trans(DenseMatrix&, const DenseMatrix&, const DenseMatrix&);

};

} // End namespace Datatypes
} // End namespace SCICore

//
// $Log$
// Revision 1.3  1999/08/25 03:48:33  sparker
// Changed SCICore/CoreDatatypes to SCICore/Datatypes
// Changed PSECore/CommonDatatypes to PSECore/Datatypes
// Other Misc. directory tree updates
//
// Revision 1.2  1999/08/17 06:38:45  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:56:21  mcq
// Initial commit
//
// Revision 1.4  1999/05/06 19:55:47  dav
// added back .h files
//
// Revision 1.1  1999/05/05 21:04:37  dav
// added SCICore .h files to /include directories
//
// Revision 1.2  1999/05/03 04:52:18  dmw
// Added and updated DaveW Datatypes/Modules
//
// Revision 1.1  1999/04/25 04:07:07  dav
// Moved files into Datatypes
//
// Revision 1.1.1.1  1999/04/24 23:12:51  dav
// Import sources
//
//

#endif
