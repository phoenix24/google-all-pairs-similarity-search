// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ---
// This class provides an absraction layer over file-based set
// (binary vector) data.
//
// The input file must have the following properties for the algorithm
// to behave correctly and/or efficiently:
//
// 1) It must consist of a list of vectors in sparse vector
// representation. The currently supported format is "apriori binary".
//
// 2) Vectors in the file are assumed to appear in increasing order of
// vector size. That is, a vector of size $i$ will always appear after
// a vector of size less than $i$.
//
// 3) Features within a vector must always appear from least to most
// frequent in a consistent order.  That is if feature $x$ appears
// less frequently than feature $y$ within the dataset, then $x$
// should always appear before $y$ within any vector containing both
// features. Furthermore if two features $x$ and $y$ have the same
// frequency, then one must be chosen to consistently appear before
// the other should they both appear in a given vector.
//
// 4) A vector must not contain duplicate features.
// ---
//  Author: Roberto Bayardo
//
#ifndef _DATA_SOURCE_ITERATOR_H_
#define _DATA_SOURCE_ITERATOR_H_

#include <stdio.h>

#include <string>
#include <vector>

#ifdef MICROSOFT   // MS VC++ specific code
typedef unsigned __int32  uint32_t;
#else
#include <stdint.h>
#endif

class DataSourceIterator {
 public:
  // Factory method for obtaining an iterator. The filepath
  // is the pathname to the file containing the data.
  static DataSourceIterator* Get(const char* filepath);
  ~DataSourceIterator();

  // Returns a human-readable string describing any error condition
  // encountered.
  std::string GetErrorMessage() { return error_; }

  // Reads the next input vector from the input file.  Returns -1 on
  // error, 0 on EOF, and 1 on success.  Checks for many dataset
  // format errors, but not all of them. For example it does not check
  // that the a vector's features are duplicate free and are
  // consistently ordered according to frequency.
  int Next(uint32_t* vector_id_, std::vector<uint32_t>* input_vector);

  bool Seek(off_t seek_offset);
  off_t Tell();

 private:
  DataSourceIterator(FILE* data);

  FILE* data_;
  uint32_t last_vector_size_;
  int lines_processed_;
  std::string error_;
};

#endif  // _DATA_SOURCE_ITERATOR_H_
