%module allpairs

%{
  #include "allpairs.h"
  #include "data-source-iterator.h"

  typedef unsigned int uint32_t;
%}

%apply unsigned int { uint32_t }

%include "allpairs.h"
%include "data-source-iterator.h"

