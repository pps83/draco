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
#include "draco/io/rbx_decoder.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <cfloat>
#include <cstdarg>

#include "draco/io/parser_utils.h"
#include "draco/metadata/geometry_metadata.h"

namespace draco {

void throw_runtime_error(const char* fmt, ...)
{
    std::string result;
    va_list argList;
    char buf[512];
    va_start(argList, fmt);
    vsnprintf(buf, sizeof(buf), fmt, argList);
    va_end(argList);
    throw std::runtime_error(buf);
}

#pragma pack(push, 1)

struct FileMeshHeader
{
    uint16_t cbSize;
    uint8_t  cbVerticesStride;
    uint8_t  cbFaceStride;
    // ---dword boundary-----
    uint32_t num_vertices;
    uint32_t num_faces;
};

struct FileMeshVertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv, tw;
    uint8_t r, g, b, a;

    FileMeshVertex()
        : vx(0.0f), vy(0.0f), vz(0.0f)
        , nx(0.0f), ny(0.0f), nz(0.0f)
        , tu(0.0f), tv(0.0f), tw(0.0f)
        , r(255), g(255), b(255), a(255)
    {}

    FileMeshVertex(float vx, float vy, float vz, float nx, float ny, float nz,
        float tu, float tv, float tw, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : vx(vx), vy(vy), vz(vz)
        , nx(nx), ny(ny), nz(nz)
        , tu(tu), tv(tv), tw(tw)
        , r(r), g(g), b(b), a(a)
    {}
};

struct FileMeshFace
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

#pragma pack(pop)

struct MeshData
{
    std::vector<FileMeshVertex> vnts;
    std::vector<FileMeshFace> faces;
};

float finf()
{
    return std::numeric_limits<float>::infinity();
}

double nan()
{
    // double is a standard type and should have quiet NaN
    return std::numeric_limits<double>::quiet_NaN();
}

bool isNaN(float x)
{
    static const float n = nan();
    return memcmp(&x, &n, sizeof(float)) == 0;
}


struct Vector3
{
    float x, y, z; // coordinates
    Vector3 unit() const
    {
        const float lenSquared = x*x + y*y + z*z;
        const float invSqrt = 1.0f / sqrtf(lenSquared);
        Vector3 ret = {x * invSqrt, y * invSqrt, z * invSqrt};
        return ret;
    }
    static bool isFinite(float x)
    {
        return !isNaN(x) && (x < finf()) && (x > -finf());
    }
    static Vector3 zero()
    {
        Vector3 v = { 0, 0, 0 };
        return v;
    }
    bool isFinite() const
    {
        return isFinite(x) && isFinite(y) && isFinite(z);
    }
};

inline unsigned int atouFast(const char* value, const char** end)
{
    const char* s = value;

    // skip whitespace
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        s++;

    // read integer part
    unsigned int result = 0;

    while (static_cast<unsigned int>(*s - '0') < 10)
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    // done!
    *end = s;

    return result;
}

inline double atofFast(const char* value, const char** end)
{
    static const double digits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    static const double powers[] = { 1e0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22 };

    const char* s = value;

    // skip whitespace
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        s++;

    // read sign
    double sign = (*s == '-') ? -1 : 1;
    s += (*s == '-' || *s == '+');

    // read integer part
    double result = 0;
    int power = 0;

    while (static_cast<unsigned int>(*s - '0') < 10)
    {
        result = result * 10 + digits[*s - '0'];
        s++;
    }

    // read fractional part
    if (*s == '.')
    {
        s++;

        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            result = result * 10 + digits[*s - '0'];
            s++;
            power--;
        }
    }

    // read exponent part
    if ((*s | ' ') == 'e')
    {
        s++;

        // read exponent sign
        int expsign = (*s == '-') ? -1 : 1;
        s += (*s == '-' || *s == '+');

        // read exponent
        int exppower = 0;

        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            exppower = exppower * 10 + (*s - '0');
            s++;
        }

        // done!
        power += expsign * exppower;
    }

    // done!
    *end = s;

    if (static_cast<unsigned int>(-power) < sizeof(powers) / sizeof(powers[0]))
        return sign * result / powers[-power];
    else if (static_cast<unsigned int>(power) < sizeof(powers) / sizeof(powers[0]))
        return sign * result * powers[power];
    else
        return sign * result * powf(10.0, power);
}

inline const char* readToken(const char* data, char terminator)
{
    while (*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n')
        ++data;

    if (*data != terminator)
        throw_runtime_error("Error reading mesh data: expected %c", terminator);

    return data + 1;
}

inline const char* readFloatToken(const char* data, char terminator, float* output)
{
    const char* end;
    double value = atofFast(data, &end);

    if (*end != terminator)
        throw_runtime_error("Error reading mesh data: expected %c", terminator);

    *output = value;

    return end + 1;
}

struct MeshVertexHasher
{
    bool operator()(const FileMeshVertex& l, const FileMeshVertex& r) const
    {
        return memcmp(&l, &r, sizeof(FileMeshVertex)) == 0;
    }

    size_t operator()(const FileMeshVertex& v) const
    {
        size_t result = 0;
        result ^= *reinterpret_cast<const uint32_t*>(&v.vx);
        result ^= *reinterpret_cast<const uint32_t*>(&v.vy);
        result ^= *reinterpret_cast<const uint32_t*>(&v.vz);
        return result;
    }
};

void reindexMesh(MeshData& mesh)
{
    std::vector<unsigned int> remap(mesh.vnts.size());

    FileMeshVertex dummy;
    dummy.vx = FLT_MAX;

    typedef std::unordered_map<FileMeshVertex, unsigned int, MeshVertexHasher, MeshVertexHasher> VertexMap;
    VertexMap vertexMap;
    vertexMap[dummy] = 0;

    for (size_t i = 0; i < mesh.vnts.size(); ++i)
    {
        unsigned int& vi = vertexMap[mesh.vnts[i]];

        if (vi == 0)
            vi = vertexMap.size();

        remap[i] = vi - 1;
    }

    std::vector<FileMeshVertex> newvnts(vertexMap.size());

    for (size_t i = 0; i < mesh.vnts.size(); ++i)
        newvnts[remap[i]] = mesh.vnts[i];

    mesh.vnts.swap(newvnts);

    for (size_t i = 0; i < mesh.faces.size(); ++i)
    {
        FileMeshFace& face = mesh.faces[i];

        face.a = remap[face.a];
        face.b = remap[face.b];
        face.c = remap[face.c];
    }
}

static MeshData* readMeshFromV1(const char* data, size_t dataSize, size_t offset_, float scaler)
{
    MeshData* mesh = new MeshData();

    const char* offset = data + offset_;
    unsigned int num_faces = atouFast(offset, &offset);

    mesh->vnts.reserve(num_faces * 3);
    mesh->faces.reserve(num_faces);

    for (unsigned int i = 0; i < num_faces; i++)
    {
        for (int v = 0; v < 3; v++)
        {
            float vx, vy, vz, nx, ny, nz, tu, tv, tw;

            offset = readToken(offset, '[');
            offset = readFloatToken(offset, ',', &vx);
            offset = readFloatToken(offset, ',', &vy);
            offset = readFloatToken(offset, ']', &vz);
            offset = readToken(offset, '[');
            offset = readFloatToken(offset, ',', &nx);
            offset = readFloatToken(offset, ',', &ny);
            offset = readFloatToken(offset, ']', &nz);
            offset = readToken(offset, '[');
            offset = readFloatToken(offset, ',', &tu);
            offset = readFloatToken(offset, ',', &tv);
            offset = readFloatToken(offset, ']', &tw);

            Vector3 v3 = { nx, ny, nz };
            Vector3 normal = v3.unit();

            if (!normal.isFinite())
                normal = Vector3::zero();

            FileMeshVertex vtx
            (
                vx * scaler, vy * scaler, vz * scaler,
                normal.x, normal.y, normal.z,
                tu, 1.f - tv, tw,
                255, 255, 255, 255
            );

            mesh->vnts.push_back(vtx);
        }

        FileMeshFace face = { i * 3 + 0, i * 3 + 1, i * 3 + 2 };

        mesh->faces.push_back(face);
    }

    reindexMesh(*mesh);

    return mesh;
}

static void readData(const char* data, size_t dataSize, size_t& offset, void* buffer, size_t size)
{
    if (offset + size > dataSize)
        throw_runtime_error("Error reading mesh data: offset is out of bounds while reading %d bytes", (int)size);

    memcpy(buffer, data + offset, size);
    offset += size;
}

static MeshData* readMeshFromV2(const char* data, size_t dataSize, size_t offset)
{
    MeshData* mesh = new MeshData;

    FileMeshHeader header;
    readData(data, dataSize, offset, &header, sizeof(header));

    if (header.cbSize != sizeof(FileMeshHeader) || header.cbFaceStride != sizeof(FileMeshFace))
        throw std::runtime_error("Error reading mesh data: incompatible stride");

    if (header.num_vertices == 0 || header.num_faces == 0)
        throw std::runtime_error("Error reading mesh data: empty mesh");

    mesh->vnts.resize(header.num_vertices);

    if (header.cbVerticesStride != sizeof(FileMeshVertex))
    {
        unsigned char vertexStride = header.cbVerticesStride < sizeof(FileMeshVertex) ? header.cbVerticesStride : sizeof(FileMeshVertex);

        if (offset + (header.num_vertices * header.cbVerticesStride) > dataSize)
            throw_runtime_error("error reading mesh data: offset is out of bounds while reading %d bytes", (int)(header.num_vertices * header.cbVerticesStride));

        for (size_t i = 0; i < header.num_vertices; ++i)
        {
            memcpy(&mesh->vnts[i], data + offset, vertexStride);
            offset += header.cbVerticesStride;
        }
    }
    else
    {
        readData(data, dataSize, offset, &mesh->vnts[0], header.num_vertices * header.cbVerticesStride);
    }

    mesh->faces.resize(header.num_faces);
    readData(data, dataSize, offset, &mesh->faces[0], header.num_faces * header.cbFaceStride);

    if (offset != dataSize)
        throw std::runtime_error("Error reading mesh data: unexpected data at end of file");

    // validate indices to avoid buffer overruns later
    for (auto& face : mesh->faces)
        if (face.a >= header.num_vertices || face.b >= header.num_vertices || face.c >= header.num_vertices)
            throw std::runtime_error("Error reading mesh data: index value out of range");

    // since we have a lot of v2 meshes that don't have a good index stream we reindex the mesh to reduce memory footprint and improve performance
    reindexMesh(*mesh);

    return mesh;
}

RbxDecoder::RbxDecoder()
    : out_mesh_(nullptr),
      out_point_cloud_(nullptr) {}

bool RbxDecoder::CheckRbxHeader(const void* data)
{
    if (0 == memcmp(data, "version 1.00", 12))
        return true;
    else if (0 == memcmp(data, "version 1.01", 12))
        return true;
    else if (0 == memcmp(data, "version 2.00", 12))
        return true;
    return false;
}

bool RbxDecoder::CheckRbxHeader(const std::string &file_name)
{
    std::ifstream fl(file_name.c_str(), std::ios::binary);
    char buf[16];
    if (!fl.read(buf, 12))
        return false;
    return CheckRbxHeader(buf);
}

bool RbxDecoder::DecodeFromFile(const std::string &file_name, Mesh *out_mesh) {
  out_mesh_ = out_mesh;
  return DecodeFromFile(file_name, static_cast<PointCloud *>(out_mesh));
}

bool RbxDecoder::DecodeFromFile(const std::string &file_name,
                                PointCloud *out_point_cloud) {
  std::ifstream file(file_name, std::ios::binary);
  if (!file)
    return false;
  // Read the whole file into a buffer.
  int64_t file_size = file.tellg();
  file.seekg(0, std::ios::end);
  file_size = file.tellg() - file_size;
  if (file_size == 0)
    return false;
  file.seekg(0, std::ios::beg);
  std::vector<char> data(file_size);
  file.read(&data[0], file_size);

  buffer_.Init(&data[0], file_size);

  out_point_cloud_ = out_point_cloud;
  return DecodeInternal();
}

bool RbxDecoder::DecodeFromBuffer(DecoderBuffer *buffer, Mesh *out_mesh) {
  out_mesh_ = out_mesh;
  return DecodeFromBuffer(buffer, static_cast<PointCloud *>(out_mesh));
}

bool RbxDecoder::DecodeFromBuffer(DecoderBuffer *buffer,
                                  PointCloud *out_point_cloud) {
  out_point_cloud_ = out_point_cloud;
  buffer_.Init(buffer->data_head(), buffer->remaining_size());
  return DecodeInternal();
}

static void writeFace(std::ostream& f, const FileMeshFace& v)
{
    f << "f "
        << v.a + 1 << '/' << v.a + 1 << '/' << v.a + 1 << ' '
        << v.b + 1 << '/' << v.b + 1 << '/' << v.b + 1 << ' '
        << v.c + 1 << '/' << v.c + 1 << '/' << v.c + 1 << '\n';
}

void WriteFileObjMesh(std::ostream& f, const MeshData& data)
{
    //f << std::fixed;
    //f << std::setprecision(6);
    f.precision(6);
    for (const auto& v : data.vnts)
        f << "v " << v.vx << ' ' << v.vy << ' ' << v.vz << '\n';
    f << '\n';
    for (const auto& v : data.vnts)
        f << "vt " << v.tu << ' ' << v.tv << ' ' << v.tw << '\n';
    f << '\n';
    for (const auto& v : data.vnts)
        f << "vn " << v.nx << ' ' << v.ny << ' ' << v.nz << '\n';
    f << '\n';
    for (const auto& v : data.faces)
        writeFace(f, v);
}

bool RbxDecoder::DecodeInternal() {
    const char* data = buffer_.data_head();
    int64_t size = buffer_.remaining_size();
    const char* versionEnd = (char*)memchr(data, '\n', size);
    if (!versionEnd)
        return false;
    size_t versionOffset = versionEnd - data + 1;
    MeshData* mesh = NULL;
    if (0 == memcmp(data, "version 1.00", 12))
        mesh = readMeshFromV1(data, (size_t)size, versionOffset, 0.5f);
    else if (0 == memcmp(data, "version 1.01", 12))
        mesh = readMeshFromV1(data, (size_t)size, versionOffset, 1.0f);
    else if (0 == memcmp(data, "version 2.00", 12))
        mesh = readMeshFromV2(data, size, versionOffset);
    if (mesh)
    {
        std::stringstream ss;
        WriteFileObjMesh(ss, *mesh);
        std::string str = ss.str();
        DecoderBuffer buffer;
        buffer.Init(str.c_str(), str.size());
        Status result = out_mesh_ ? ObjDecoder::DecodeFromBuffer(&buffer, out_mesh_)
            : ObjDecoder::DecodeFromBuffer(&buffer, out_point_cloud_);
        delete mesh;
        return result.ok();
    }
    return false;
}


}  // namespace draco
