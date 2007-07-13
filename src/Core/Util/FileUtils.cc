/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


/* FileUtils.cc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#  include <unistd.h>
#  include <dirent.h>
#else
#  include <io.h>
   typedef unsigned short mode_t;
#  define MAXPATHLEN 256
#endif

#include <sys/stat.h>
#include <Core/Util/FileUtils.h>
#include <Core/OS/Dir.h>

namespace SCIRun {

  using std::string;
  using std::map;
  using std::pair;

/* Normally, I would just use sed via system() to edit a file,
   but for some reason system() calls never work from Dataflow 
   processes in linux.  Oh well, sed isn't natively available
   under windows, so I'd have to do something like this anyhow
   - Chris Moulding */ 

void
InsertStringInFile(char* filename, char* match, char* replacement)
{
  char* newfilename = new char[strlen(filename)+2];
  char c;
  sprintf(newfilename,"%s~",filename);
  FILE* ifile;
  FILE* ofile;

  /* create a copy of the original file */
  ifile = fopen(filename,"r");
  ofile = fopen(newfilename,"w");

  c = (char)fgetc(ifile);
  while (c!=(char)EOF) {
    fprintf(ofile,"%c",c);
    c = (char)fgetc(ifile);
  }
  fclose(ifile);
  fclose(ofile);

  /* search the copy for an instance of "match" */
  int index1 = 0;
  unsigned int index2 = 0;
  int foundat = -1;
  ifile = fopen(newfilename,"r");
  c = (char)fgetc(ifile);
  while (c!=(char)EOF) {
    if (c==match[index2]) {
      foundat = index1;
      while (index2<strlen(match) && c!=(char)EOF && c==match[index2]) {
        c = (char)fgetc(ifile);
        index1++;
        index2++;
      }
      if (foundat>=0 && index2!=strlen(match)) {
        foundat = -1;
        index2 = 0;
      } else
        break;
    }
    c = (char)fgetc(ifile);
    index1++;
  }
  fclose(ifile);

  /* if an instance of match was found, 
     insert the indicated string */
  if (foundat>=0) {
    index1 = 0;
    ifile = fopen(newfilename,"r");
    ofile = fopen(filename,"w");
    c = (char)fgetc(ifile);
    while (c!=(char)EOF) {
      if (index1==foundat)
        fprintf(ofile,"%s",replacement);
      fprintf(ofile,"%c",c);
      c = (char)fgetc(ifile);
      index1++;
    }
    fclose(ifile);
    fclose(ofile);
  } 
}

#if 0
void InsertStringInFile(char* filename, char* match, char* replacement)
{
  char* string = 0;
  char* mod = 0;

  mod = new char[strlen(replacement)+strlen(match)+25];
  sprintf(mod,"%s%s",replacement,match);

  string = new char[strlen(match)+strlen(replacement)+100];
  sprintf(string,"sed -e 's,%s,%s,g' %s > %s.mod &\n",match,mod,
          filename,filename);
  system(string);

  sprintf(string,"mv -f %s.mod %s\n",filename,filename);
  system(string);

  delete[] string;
  delete[] mod;
}
#endif

map<int,char*>*
GetFilenamesEndingWith(char* d, char* ext)
{
  map<int,char*>* newmap = 0;
  dirent* file = 0;
  DIR* dir = opendir(d);
  char* newstring = 0;

  if (!dir) 
    return 0;

  newmap = new map<int,char*>;

  file = readdir(dir);
  while (file) {
    if ((strlen(file->d_name)>=strlen(ext)) && 
        (strcmp(&(file->d_name[strlen(file->d_name)-strlen(ext)]),ext)==0)) {
      newstring = new char[strlen(file->d_name)+1];
      sprintf(newstring,"%s",file->d_name);
      newmap->insert(pair<int,char*>(newmap->size(),newstring));
    }
    file = readdir(dir);
  }

  closedir(dir);
  return newmap;
}

bool
validFile( string filename ) 
{
  struct stat buf;
  if (stat(filename.c_str(), &buf) == 0)
  {
    mode_t &m = buf.st_mode;
    return (m & S_IRUSR && S_ISREG(m) && !S_ISDIR(m));
  }
  return false;
}

bool
validDir( string dirname ) 
{
  struct stat buf;
  if (stat(dirname.c_str(), &buf) == 0)
  {
    mode_t &m = buf.st_mode;
    return (m & S_IRUSR && !S_ISREG(m) && S_ISDIR(m));
  }
  return false;
}
    
bool
isSymLink( string filename ) 
{
  struct stat buf;
  if( lstat(filename.c_str(), &buf) == 0 )
  {
    mode_t &m = buf.st_mode;
    return( m & S_ISLNK(m) );
  }
  return false;
}

// Creates a temp file (in directoryPath), writes to it, and then deletes it...
bool
testFilesystem( string directoryPath )
{
  FILE * fp;

  string fileName = directoryPath + "/scirun_filesystem_check_temp_file";

  // Create a temporary file
  fp = fopen( fileName.c_str(), "w" );
  if( fp == NULL ) {
    printf( "ERROR: testFilesystem() failed to create a temporary file in %s\n", directoryPath.c_str() );
    printf( "       errno is %d: %s\n", errno, strerror(errno) );
      return false;
  }

  // Write to the file
  char * myStr = "hello world";
  for( int cnt = 0; cnt < 1000; cnt++ ) {
    int numWritten = fwrite( myStr, 1, 11, fp );
    if( numWritten != 11 ) {
      printf( "ERROR: testFilesystem() failed to write data to temp file in %s\n", directoryPath.c_str() );
      printf( "       iteration: %d, errno is %d: %s\n", cnt, errno, strerror(errno) );
      return false;
    }
  }

  // Close the file
  int result = fclose( fp );
  if (result != 0) {
    printf( "WARNING: fclose() failed while testing filesystem.\n" );
    printf( "         errno is %d: %s\n", errno, strerror(errno) );
    return false;
  }

  // Check the files size
  struct stat buf;
  if( stat(fileName.c_str(), &buf) == 0 )
  {
    printf( "FILESYSTEM CHECK: Test file size is: %d\n", buf.st_size );
  } else {
    printf( "WARNING: stat() failed while testing filesystem.\n" );
    printf( "         errno is %d: %s\n", errno, strerror(errno) );
    return false;
  }

  // Delete the file
  int rc = remove( fileName.c_str() );
  if (rc != 0) {
    printf( "WARNING: remove() failed while testing filesystem.\n" );
    printf( "         errno is %d: %s\n", errno, strerror(errno) );
    return false;
  }
  return true;
}

} // End namespace SCIRun
