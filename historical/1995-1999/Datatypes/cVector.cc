#include <Datatypes/cVector.h>

PersistentTypeID cVector::type_id("cVector", "Datatype", 0);

//-----------------------------------------------------------------
cVector::cVector(int N){
 int i;
 Size = N;

 a = new Complex [N];

 for(i=0;i<N;i++)
          a[i] = 0;
}

//----------------------------------------------------------------
cVector::~cVector(){

 delete [] a;
}

//----------------------------------------------------------------
cVector::cVector(const cVector &B){
 int i;
 int N = B.Size;

 a = new Complex [N];

 Size = B.Size;

 for(i=0;i<N;i++)
    a[i] = B.a[i];
}

//----------------------------------------------------------------
cVector &cVector::operator=(const cVector &B){
 int i;
 int N = B.Size;
  
 delete [] a;

 a = new Complex [N];

 Size = B.Size;

 for(i=0;i<N;i++)
  a[i] = B.a[i];

 return *this;
}

//-----------------------------------------------------------------
Complex& cVector::operator()(int i){

 if((i>=0)&&(i<Size)){
  return(a[i]);
  }

 else 
    cerr <<"Error: cVector index is out of range!"<<endl;
 return(a[0]);  
}

//-----------------------------------------------------------------
cVector  cVector:: operator+(const cVector& B) const{
 cVector C(Size);

 for(int i = 0;i < Size;i++)
        C.a[i] = a[i] + B.a[i];

 return (C);
}

//-----------------------------------------------------------------
cVector cVector:: operator-(const cVector& B) const{
 cVector C(Size);

 for(int i = 0;i < Size;i++)
         C.a[i] = a[i] - B.a[i];
 
 return (C);
}
//-----------------------------------------------------------------
double cVector::norm(){

double norm2=0;

 for(int i = 0;i < Size;i++)
       
         norm2 = norm2 + a[i].abs()*a[i].abs();
 
 return (sqrt(norm2));
}

//-----------------------------------------------------------------
int cVector::load(char* filename){

 ifstream file_in(filename);

 if(!file_in){
      cerr<<"Error:Cannot open input file:" <<filename<<endl;
  return(-1);
}

 for(int i = 0;i < Size;i++)
       file_in>>a[i];


 file_in.close();
 return(0); 
}

//-----------------------------------------------------------------
void cVector::set(const cVector& B){
  for(int i=0;i<Size;i++)
    a[i] = B.a[i];    
}

//-----------------------------------------------------------------
void cVector::add(const cVector& B){
  for(int i=0;i<Size;i++)
    a[i] = a[i] + B.a[i];    
}

//-----------------------------------------------------------------
void cVector::subtr(const cVector& B){
  for(int i=0;i<Size;i++)
    a[i] = a[i] - B.a[i];    
}

//-----------------------------------------------------------------
void cVector::mult(const Complex x){
  for(int i=0;i<Size;i++)
    a[i] = x*a[i];    
}

//-----------------------------------------------------------------
void cVector::conj(){
  for(int i=0;i<Size;i++)
    a[i] = a[i].conj();    
}

//------------------------------------------------------------------
Complex operator* (cVector& A, cVector& B){
Complex S;
  S = 0;

 for(int i=0;i<A.Size;i++)
     S = S + ((A.a[i]).conj())*B.a[i];

 return (S);
}

//-----------------------------------------------------------------
ostream &operator<< (ostream &output, cVector  &A){

 output<<"[";

 for(int i=0 ;i < A.size();i++)
     output<<A.a[i]<<" ";

 output<<"]";
 output<<endl;

 return(output);
}

//-----------------------------------------------------------------
cVector operator*(Complex x,const cVector &B){
 cVector  C(B.Size);

 for(int i=0;i<B.Size;i++)
  C.a[i] = x*B.a[i];

 return (C);
}

//-----------------------------------------------------------------
cVector operator*(const cVector &B,Complex x){
 cVector  C(B.Size);

 for(int i=0;i<B.Size;i++)
  C.a[i]=x*B.a[i];

 return (C);
}
//-----------------------------------------------------------------
cVector operator*(double x,const cVector &B){
 cVector  C(B.Size);

 for(int i=0;i<B.Size;i++)
  C.a[i] = x*B.a[i];

 return (C);
}

//-----------------------------------------------------------------
cVector operator*(const cVector &B,double x){
 cVector  C(B.Size);

 for(int i=0;i<B.Size;i++)
  C.a[i]=x*B.a[i];

 return (C);
}


//-----------------------------------------------------------------


void cVector::io(Piostream&) {
  cerr << "cVector::io not finished\n";
}



