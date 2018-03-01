// Copyright 2016 The Draco Authors.
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
#include "draco/core/options.h"

#include <cstdlib>
#include <string>

namespace draco {

Options::Options() {}

void Options::SetInt(const std::string &name, int val) {
  options_[name] = std::to_string(val);
}

void Options::SetFloat(const std::string &name, float val) {
  options_[name] = std::to_string(val);
}

void Options::SetBool(const std::string &name, bool val) {
  options_[name] = std::to_string(val ? 1 : 0);
}

void Options::SetString(const std::string &name, const std::string &val) {
  options_[name] = val;
}

int Options::GetInt(const std::string &name) const { return GetInt(name, -1); }

int Options::GetInt(const std::string &name, int default_val) const {
  const auto it = options_.find(name);
  if (it == options_.end())
    return default_val;
  return std::atoi(it->second.c_str());
}

float Options::GetFloat(const std::string &name) const {
  return GetFloat(name, -1);
}

float Options::GetFloat(const std::string &name, float default_val) const {
  const auto it = options_.find(name);
  if (it == options_.end())
    return default_val;
  return std::atof(it->second.c_str());
}

bool Options::GetBool(const std::string &name) const {
  return GetBool(name, false);
}

bool Options::GetBool(const std::string &name, bool default_val) const {
  const int ret = GetInt(name, -1);
  if (ret == -1)
    return default_val;
  return static_cast<bool>(ret);
}

std::string Options::GetString(const std::string &name) const {
  return GetString(name, "");
}

std::string Options::GetString(const std::string &name,
                               const std::string &default_val) const {
  const auto it = options_.find(name);
  if (it == options_.end())
    return default_val;
  return it->second;
}

template <typename DataTypeT>
void Options::SetVector(const std::string &name, const DataTypeT *vec,
                        int num_dims) {
  std::string out;
  for (int i = 0; i < num_dims; ++i) {
    if (i > 0)
      out += " ";

    out += std::to_string(vec[i]);
  }
  options_[name] = out;
}

template <class VectorT>
VectorT Options::GetVector(const std::string &name,
                           const VectorT &default_val) const {
  VectorT ret = default_val;
  GetVector(name, VectorT::dimension, &ret[0]);
  return ret;
}

template <typename DataTypeT>
bool Options::GetVector(const std::string &name, int num_dims,
                        DataTypeT *out_val) const {
  const auto it = options_.find(name);
  if (it == options_.end())
    return false;
  const std::string value = it->second;
  if (value.length() == 0)
    return true;  // Option set but no data is present
  const char *act_str = value.c_str();
  char *next_str;
  for (int i = 0; i < num_dims; ++i) {
    if (std::is_integral<DataTypeT>::value) {
      const int val = std::strtol(act_str, &next_str, 10);
      if (act_str == next_str)
        return true;  // End reached.
      act_str = next_str;
      out_val[i] = static_cast<DataTypeT>(val);
    } else {
      const float val = std::strtof(act_str, &next_str);
      if (act_str == next_str)
        return true;  // End reached.
      act_str = next_str;
      out_val[i] = static_cast<DataTypeT>(val);
    }
  }
  return true;
}

template bool Options::GetVector<float>(const std::string &name,
  int num_dims, float *out_val) const;

template void Options::SetVector<float>(const std::string &name,
  const float *vec, int num_dims);

}  // namespace draco
