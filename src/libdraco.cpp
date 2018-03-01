#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#pragma warning(disable: 4018 4146 4244 4267 4661 4800)
#endif

#define DRACO_POINT_CLOUD_COMPRESSION_SUPPORTED
#define DRACO_MESH_COMPRESSION_SUPPORTED
#define DRACO_STANDARD_EDGEBREAKER_SUPPORTED
#define DRACO_PREDICTIVE_EDGEBREAKER_SUPPORTED
#define DRACO_BACKWARDS_COMPATIBILITY_SUPPORTED
#define DRACO_ATTRIBUTE_DEDUPLICATION_SUPPORTED

#ifdef __ANDROID__
#define _USE_MATH_DEFINES
#include <cmath>
#include <ios>
#include <string>
#include <cstdlib>
#include <android/api-level.h>
#if __ANDROID_API__ < 18
static inline double log2(double n)
{
    return log(n) * M_LOG2E;
}
#endif

namespace std
{
    static inline string to_string(float val_) // convert float to string
    {
#ifdef _MSC_VER
        int len_ = _CSTD _scprintf("%f", val_);
        string str_(len_ + 1, '\0');
        _CSTD sprintf_s(&str_[0], len_ + 1, _Fmt, val_);
#else
        int len_ = snprintf(NULL, 0, "%f", val_);
        string str_(len_ + 1, '\0');
        snprintf(&str_[0], len_ + 1, "%f", val_);
#endif
        str_.resize(len_);
        return str_;
    }

    static inline string to_string(int val_) // convert int to string
    {
        string str_(21, '\0');
#ifdef _MSC_VER
        int len_ = _CSTD sprintf_s(&str_[0], 21, "%d", val_);
#else
        int len_ = snprintf(&str_[0], 21, "%d", val_);
#endif
        str_.resize(len_);
        return str_;
    }

    using ::strtof;
}
#endif

#include "draco/core/cycle_timer.cc"
#include "draco/core/data_buffer.cc"
#include "draco/core/decoder_buffer.cc"
#include "draco/core/divide.cc"
#include "draco/core/draco_types.cc"
#include "draco/core/encoder_buffer.cc"
#include "draco/core/hash_utils.cc"
#include "draco/core/options.cc"
#include "draco/core/quantization_utils.cc"
#include "draco/core/shannon_entropy.cc"
#include "draco/core/symbol_coding_utils.cc"

#include "draco/core/symbol_decoding.cc"
#include "draco/core/symbol_encoding.cc"
#include "draco/core/bit_coders/adaptive_rans_bit_decoder.cc"
#include "draco/core/bit_coders/adaptive_rans_bit_encoder.cc"
#include "draco/core/bit_coders/direct_bit_decoder.cc"
#include "draco/core/bit_coders/direct_bit_encoder.cc"
#include "draco/core/bit_coders/rans_bit_decoder.cc"
#include "draco/core/bit_coders/rans_bit_encoder.cc"
#include "draco/core/bit_coders/symbol_bit_decoder.cc"
#include "draco/core/bit_coders/symbol_bit_encoder.cc"

#include "draco/io/mesh_io.cc"
#include "draco/io/obj_decoder.cc"
#include "draco/io/obj_encoder.cc"
#include "draco/io/parser_utils.cc"
#include "draco/io/ply_decoder.cc"
#include "draco/io/ply_encoder.cc"
#include "draco/io/ply_reader.cc"
#include "draco/io/point_cloud_io.cc"
#include "draco/io/rbx_decoder.cc"

#include "draco/compression/attributes/attributes_decoder.cc"
#include "draco/compression/attributes/kd_tree_attributes_decoder.cc"
#include "draco/compression/attributes/sequential_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_attribute_decoders_controller.cc"
#include "draco/compression/attributes/sequential_integer_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_normal_attribute_decoder.cc"
#include "draco/compression/attributes/sequential_quantization_attribute_decoder.cc"
#include "draco/compression/attributes/attributes_encoder.cc"
#include "draco/compression/attributes/kd_tree_attributes_encoder.cc"
#include "draco/compression/attributes/sequential_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_attribute_encoders_controller.cc"
#include "draco/compression/attributes/sequential_integer_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_normal_attribute_encoder.cc"
#include "draco/compression/attributes/sequential_quantization_attribute_encoder.cc"
#include "draco/compression/attributes/prediction_schemes/prediction_scheme_encoder_factory.cc"
#include "draco/compression/expert_encode.cc"
#include "draco/compression/mesh/mesh_edgebreaker_encoder_impl.cc"
#include "draco/compression/mesh/mesh_edgebreaker_decoder_impl.cc"
#include "draco/compression/mesh/mesh_decoder.cc"
#include "draco/compression/mesh/mesh_edgebreaker_decoder.cc"
#include "draco/compression/mesh/mesh_sequential_decoder.cc"
#include "draco/compression/mesh/mesh_edgebreaker_encoder.cc"
#include "draco/compression/mesh/mesh_encoder.cc"
#include "draco/compression/mesh/mesh_sequential_encoder.cc"
#include "draco/compression/point_cloud/point_cloud_decoder.cc"
#include "draco/compression/point_cloud/point_cloud_kd_tree_decoder.cc"
#include "draco/compression/point_cloud/point_cloud_sequential_decoder.cc"
#include "draco/compression/point_cloud/point_cloud_encoder.cc"
#include "draco/compression/point_cloud/point_cloud_kd_tree_encoder.cc"
#include "draco/compression/point_cloud/point_cloud_sequential_encoder.cc"
#include "draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_decoder.cc"
#include "draco/compression/point_cloud/algorithms/float_points_tree_decoder.cc"
#include "draco/compression/point_cloud/algorithms/dynamic_integer_points_kd_tree_encoder.cc"
#include "draco/compression/point_cloud/algorithms/float_points_tree_encoder.cc"
#include "draco/compression/decode.cc"
#include "draco/compression/encode.cc"

#include "draco/attributes/attribute_octahedron_transform.cc"
#include "draco/attributes/attribute_quantization_transform.cc"
#include "draco/attributes/attribute_transform.cc"
#include "draco/attributes/geometry_attribute.cc"
#include "draco/attributes/point_attribute.cc"

#include "draco/mesh/corner_table.cc"
#include "draco/mesh/mesh.cc"
#include "draco/mesh/mesh_are_equivalent.cc"
#include "draco/mesh/mesh_attribute_corner_table.cc"
#include "draco/mesh/mesh_cleanup.cc"
#include "draco/mesh/mesh_misc_functions.cc"
#include "draco/mesh/mesh_stripifier.cc"
#include "draco/mesh/triangle_soup_mesh_builder.cc"

#include "draco/metadata/geometry_metadata.cc"
#include "draco/metadata/metadata.cc"
#include "draco/metadata/metadata_decoder.cc"
#include "draco/metadata/metadata_encoder.cc"

#include "draco/point_cloud/point_cloud.cc"
#include "draco/point_cloud/point_cloud_builder.cc"
