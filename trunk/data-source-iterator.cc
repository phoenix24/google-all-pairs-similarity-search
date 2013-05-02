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
// Provides an absraction layer over file-based set (binary vector) data.
// ---
// Author: Roberto Bayardo

#include "data-source-iterator.h"

#include <errno.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#ifdef MICROSOFT
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace {

// If a vector is encountered with size larger than this constant,
// then we will barf. This is to protect against memory overflow from
// improperly formatted binary data.
const uint32_t kMaxVectorSize = 99999;

std::string ToString(uint32_t l) {
  char buf[30];
  sprintf(buf, "%u", l);
  return std::string(buf);
}

}  // namespace

/*static*/
DataSourceIterator* DataSourceIterator::Get(const char* filename) {
  FILE* data = fopen(filename, "rb");
  if (!data) {
    std::cerr << "ERROR: Failed to open input file ("
              << filename << "): " << strerror(errno) << "\n";
    return 0;
  }
  return new DataSourceIterator(data);
}

DataSourceIterator::DataSourceIterator(FILE* data)
    : data_(data),
      last_vector_size_(0),
      lines_processed_(0) {
}

DataSourceIterator::~DataSourceIterator() {
  fclose(data_);
  data_ = 0;
}

bool DataSourceIterator::Seek(off_t resume_offset) {
  if (fseeko(data_, resume_offset, 0)) {
    error_ = "fseek failed: " +  std::string(strerror(errno));
    return false;
  }
  last_vector_size_ = 0;
  return true;
}

off_t DataSourceIterator::Tell() {
  return ftello(data_);
}

int DataSourceIterator::Next(uint32_t* vector_id, std::vector<uint32_t>* vec) {
  size_t bytes_read;
  uint32_t vector_size;

  while ((bytes_read = fread(vector_id, 1, 4, data_)) == 4) {
    bytes_read = fread(&vector_size, 1, 4, data_);
    if (bytes_read != 4) {
      if (ferror(data_))
        break;
      error_ = "Dataset format error. Partial vector length encountered "
          "for vector id " + ToString(*vector_id);
      return -1;
    }
    if (vector_size > kMaxVectorSize) {
      error_ = "Dataset format error. Size of vector id " +
          ToString(*vector_id) +
          " exceeds maximum: " +
          ToString(vector_size);
      return -1;
    }
    if (vector_size < last_vector_size_) {
      error_ = "Dataset format error. Size of vector id " +
          ToString(*vector_id) +
          " is smaller than that of the preceeding vector: " +
          ToString(vector_size);
      return -1;
    }
    last_vector_size_ = vector_size;
    vec->resize(vector_size);
    bytes_read = fread(&((*vec)[0]), 1, 4 * vector_size, data_);
    if (bytes_read != 4 * vector_size) {
      if (ferror(data_))
        break;
      error_ = "Dataset format error. Dataset truncated while reading "
          "features from vector id " +
          ToString(*vector_id);
      return -1;
    }
    lines_processed_++;
    return 1;
  }
  if (ferror(data_)) {
    error_ = "Dataset read error, ferror code=" + ToString(ferror(data_));
    return -1;
  }
  if (bytes_read != 0) {
    error_ = "Dataset format error. Partial vector id encountered.";
    return -1;
  }
  return 0;
}
