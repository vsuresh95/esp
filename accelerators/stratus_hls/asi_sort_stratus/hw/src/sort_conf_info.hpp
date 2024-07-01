// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SORT_CONF_INFO_HPP__
#define __SORT_CONF_INFO_HPP__

#include <systemc.h>

//
// Configuration parameters for the accelerator.
//
class conf_info_t
{
public:
  uint32_t len; // Length of vectors to sort
  uint32_t batch; // Number of vectors to sort
  int32_t prod_valid_offset;
  int32_t prod_ready_offset;
  int32_t cons_valid_offset;
  int32_t cons_ready_offset;
  int32_t input_offset;
  int32_t output_offset;

  //
  // constructors
  //
  conf_info_t()
  {
    this->len = 0;
    this->batch = 0;
    this->prod_valid_offset = 0;
    this->prod_ready_offset = 0;
    this->cons_valid_offset = 0;
    this->cons_ready_offset = 0;
    this->input_offset = 0;
    this->output_offset = 0;
  }

  conf_info_t(
    uint32_t l,
    uint32_t b,
    int32_t prod_valid_offset,
    int32_t prod_ready_offset,
    int32_t cons_valid_offset,
    int32_t cons_ready_offset,
    int32_t input_offset,
    int32_t output_offset)
  {
    this->len = l;
    this->batch = b;
    this->prod_valid_offset = prod_valid_offset;
    this->prod_ready_offset = prod_ready_offset;
    this->cons_valid_offset = cons_valid_offset;
    this->cons_ready_offset = cons_ready_offset;
    this->input_offset = input_offset;
    this->output_offset = output_offset;
  }

  // equals operator
  inline bool operator==(const conf_info_t &rhs) const
  {
      if (len != rhs.len) return false;
      if (batch != rhs.batch) return false;
      if (prod_valid_offset != rhs.prod_valid_offset) return false;
      if (prod_ready_offset != rhs.prod_ready_offset) return false;
      if (cons_valid_offset != rhs.cons_valid_offset) return false;
      if (cons_ready_offset != rhs.cons_valid_offset) return false;
      if (input_offset != rhs.input_offset) return false;
      if (output_offset != rhs.output_offset) return false;
      return true;
  }

  // assignment operator
  inline conf_info_t& operator=(const conf_info_t& other)
  {
    len = other.len;
    batch = other.batch;
    prod_valid_offset = other.prod_valid_offset;
    prod_ready_offset = other.prod_ready_offset;
    cons_valid_offset = other.cons_valid_offset;
    cons_ready_offset = other.cons_ready_offset;
    input_offset = other.input_offset;
    output_offset = other.output_offset;

    return *this;
  }

  // VCD dumping function
  friend void sc_trace(sc_trace_file *tf, const conf_info_t &v, const std::string &NAME)
  {}

  // redirection operator
  friend ostream& operator << (ostream& os, conf_info_t const &conf_info)
  {
    os << "{ len = " << conf_info.len;
    os<< ", batch = " << conf_info.batch;
    os << "prod_valid_offset = " << conf_info.prod_valid_offset << ", ";
    os << "prod_ready_offset = " << conf_info.prod_ready_offset << ", ";
    os << "cons_valid_offset = " << conf_info.cons_valid_offset << ", ";
    os << "cons_ready_offset = " << conf_info.cons_ready_offset << ", ";
    os << "input_offset = " << conf_info.input_offset << ", ";
    os << "output_offset = " << conf_info.output_offset << "";
    os << "}";
    return os;
  }
};

#endif // __SORT_CONF_INFO_HPP__
