#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logger.h"
#include "utils.h"

#include "actor_generated.h"
#include "animation_generated.h"
#include "mesh_generated.h"
#include "resources.h"
#include "skeleton_generated.h"
#include "terrain_generated.h"
#include "texture_generated.h"

#include "flatbuffers/flatbuffers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#include "gli/gli.hpp"
#pragma GCC diagnostic pop

#include "lodepng/lodepng.h"
#include "tinyxml2/tinyxml2.h"
#include "libimagequant.h"
#include "0ad/decompose.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
#pragma GCC diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
// Include order is important because FCollada headers have a lot of missing includes.
#include <FCollada.h>
#include <FUtils/Platforms.h>
#include <FUtils/FUtils.h>
#include <FCDocument/FCDocument.h>
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include <FCDocument/FCDAsset.h>
#include <FCDocument/FCDController.h>
#include <FCDocument/FCDControllerInstance.h>
#include <FCDocument/FCDSceneNode.h>
#include <FCDocument/FCDSkinController.h>
#include <FCDocument/FCDocument.h>
#include <FCDocument/FCDGeometry.h>
#include <FCDocument/FCDGeometryMesh.h>
#include <FCDocument/FCDGeometryPolygons.h>
#include <FCDocument/FCDGeometryPolygonsInput.h>
#include <FCDocument/FCDGeometryPolygonsTools.h>
#include <FCDocument/FCDGeometrySource.h>
#pragma GCC diagnostic pop

using TinyXMLDocument = tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLHandle;
using tinyxml2::XMLNode;
using tinyxml2::XMLText;

namespace {
constexpr int kFlatBuilderInitSize = 4 * 1024 * 1024;

class DoneTracker {
 public:
  bool ShouldSkip(const std::string& s) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (done_set_.find(s) != done_set_.end()) {
      return true;
    } else {
      done_set_.insert(s);
      return false;
    }
  }

 private:
  std::mutex mutex_;
  std::set<std::string> done_set_;
};

class ThreadPool {
 public:
  ThreadPool(int num_threads) : num_threads_(num_threads), done_(false) {}
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Tell the workers to exit as soon as the work_queue_ is empty.
  void SetDone() { done_ = true; cv_.notify_all(); }

  void Push(std::function<void()> work) {
    std::lock_guard<std::mutex> lock(mutex_);
    work_queue_.push(work);
    cv_.notify_one();
  }

  void Run() {
    auto worker_fn = [&]() {
      while (true) {
        std::function<void()> work;
        bool all_done = false;

        {
          std::unique_lock<std::mutex> lock(mutex_);
          cv_.wait(lock, [&]{ return done_ || !work_queue_.empty(); });

          if (!work_queue_.empty()) {
            work = work_queue_.front();
            work_queue_.pop();
            if (work_queue_.empty() && done_) {
              // We took the last task, so we should notify everyone to wakeup and die (once we release the mutex).
              all_done = true;
            }
          } else if (done_) {
            break;
          }
        }
        if (all_done) {
          cv_.notify_all();
        }
        if (work) {
          work();
        }
      }
    };

    for (int i = 0; i < num_threads_; ++i) {
      threads_.push_back(std::thread(worker_fn));
    }
  }

  void JoinAll() {
    for (std::thread& t : threads_) {
      t.join();
    }
    threads_.clear();
  }

  ~ThreadPool() {
    SetDone();
    JoinAll();
  }

 private:
  int num_threads_;
  bool done_;
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> work_queue_;
  std::condition_variable cv_;
  std::mutex mutex_;
};

std::vector<XMLHandle> GetAllChildrenElements(XMLHandle root, const std::string& elem) {
  std::vector<XMLHandle> ret;
  XMLHandle it = root.FirstChildElement(elem.c_str());
  while (it.ToNode() != nullptr) {
    ret.push_back(it);
    it = it.NextSiblingElement(elem.c_str());
  }
  return ret;
}

void MkdirIfNotExist(const std::string& dir) {
  // TODO: Do this properly.
  #if defined(_WIN32)
  std::string dir_corrected = dir;
  for (char& c : dir_corrected) {
    if (c == '/') {
      c = '\\';
    }
  }
  std::string command = std::string("if not exist ") +
      dir_corrected + " mkdir " + dir_corrected;
  system(command.c_str());
  #else
  system((std::string("mkdir -p ") + dir).c_str());
  #endif
}

std::string RemoveExtension(const std::string& s) {
  return s.substr(0, s.find_last_of('.'));
}

std::string Extension(const std::string& s) {
  return s.substr(s.find_last_of('.') + 1);
}

void WriteToFile(const std::string& path, const uint8_t* buf, std::size_t size) {
  std::size_t last_separator = path.find_last_of('/');
  std::string dir = path.substr(0, last_separator);
  std::string filename = path.substr(last_separator + 1);
  MkdirIfNotExist(dir);
  std::ofstream of(
      path.c_str(),
      std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  of.write(reinterpret_cast<const char*>(buf), size);
}

void WriteFB(const std::string& path_stem, const flatbuffers::FlatBufferBuilder& builder) {
  WriteToFile(path_stem + ".fb", builder.GetBufferPointer(), builder.GetSize());
}

XMLNode* DeepCopy(XMLNode* node, TinyXMLDocument* doc) {
    XMLNode* ret = node->ShallowClone(doc);
    for (XMLNode* child = node->FirstChild(); child != nullptr; child = child->NextSibling()) {
      ret->InsertEndChild(DeepCopy(child, doc));
    }

    return ret;
}

// Variants can have a "file" attribute that "includes" another file.
// Here we copy the file into the main tree (if applicable).
void ProcessVariantIncludes(XMLElement* element, TinyXMLDocument* doc) {
  if (!element) {
    return;
  }
  
  const char* file_attrib = element->Attribute("file");
  if (!file_attrib) {
    return;
  }

  std::string full_path = std::string(kInputPrefix) + kVariantPathPrefix + file_attrib;
  TinyXMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open for variant inclusion: ") + full_path);
  }

  XMLElement* root = xml_doc.FirstChildElement("variant");
  if (!root) {
    throw std::runtime_error(std::string("Variant include file ") + full_path + " does not have <variant> root");
  }
  ProcessVariantIncludes(root, doc);

  // Copy attributes over.
  auto attr = root->FirstAttribute();
  while (attr != nullptr) {
    element->SetAttribute(attr->Name(), attr->Value());
    attr = attr->Next();
  }

  XMLNode* child = root->FirstChild();
  while (child) {
    element->InsertEndChild(DeepCopy(child, doc));
    child = child->NextSibling();
  }
}

class ColladaDocument {
 public:
  ColladaDocument() {
    doc_ = FCollada::NewTopDocument();
  }

  ~ColladaDocument() {
    SAFE_RELEASE(doc_);
  }

  ColladaDocument(const ColladaDocument&) = delete;
  ColladaDocument& operator=(const ColladaDocument&) = delete;

  // Returns true for success.
  bool LoadFromText(const std::string& text) {
    content_ = text;

    // Unfortunately we have to serialize this because FCollada doesn't seem to be thread-safe. But it seems
    // to be fine as long as we guard the loading part? Not sure. We can enlarge the critical section if
    // we get more crashes. I think it's due to libxml not being thread safe. Unfortunately it does take up
    // a large portion of the total run time.
    {
      static std::mutex mutex;
      std::lock_guard<std::mutex> lock(mutex);
      if (!FCollada::LoadDocumentFromMemory("unknown.dae", doc_, reinterpret_cast<void*>(content_.data()), text.size())) {
        return false;
      }
    }

    // 0ad has support for files generated with XSI, but there doesn't seem to be any, so we don't support it.
    FCDAsset* asset = doc_->GetAsset();
    if (asset && asset->GetContributorCount() >= 1) {
      std::string tool(asset->GetContributor(0)->GetAuthoringTool());
      if (tool.find("XSI") != tool.npos) {
        throw std::runtime_error("XSI files aren't supported");
      }
    }

    return true;
  }

  FCDocument* Doc() { return doc_; }

 private:
  FCDocument* doc_;
  std::string content_;
};

// Instances in a scene can be optionally marked for export for disambiguation.
struct TransformedInstance {
  FCDEntityInstance* instance;
  FMMatrix44 transform;
  bool export_marked;
};

// Find instances in the tree. The logic here is basically copied from
// https://github.com/0ad/0ad/blob/01126b9f328bdbee029765a86f2ad2215c476448/source/collada/CommonConvert.cpp#L232
// But we handle the disambiguation slightly differently.
std::vector<TransformedInstance> FindInstances(FCDSceneNode* node, const FMMatrix44& transform) {
  std::vector<TransformedInstance> found;

  FMMatrix44 node_transform = transform * node->ToMatrix();

  for (std::size_t i = 0; i < node->GetChildrenCount(); ++i) {
    auto* child = node->GetChild(i);
    auto child_instances = FindInstances(child, node_transform);
    found.insert(found.end(),
                 std::make_move_iterator(child_instances.begin()),
                 std::make_move_iterator(child_instances.end()));
  }

  // TODO: try XSI property.
  bool is_visible = (node->GetVisibility() > 0.0f);

  if (!is_visible) {
    return found;
  }

  bool marked = node->GetNote() == "export";

  for (std::size_t i = 0; i < node->GetInstanceCount(); ++i) {
    FCDEntity::Type type = node->GetInstance(i)->GetEntityType();

    if (type != FCDEntity::GEOMETRY && type != FCDEntity::CONTROLLER) {
      continue;
    }

    TransformedInstance t_instance;
    t_instance.instance = node->GetInstance(i);
    t_instance.transform = node_transform;
    t_instance.export_marked = marked;
    found.push_back(std::move(t_instance));
  }

  return found;
}

TransformedInstance FindSingleInstance(FCDSceneNode* node) {
  auto all_instances = FindInstances(node, FMMatrix44::Identity);
  auto is_marked = [](const TransformedInstance& t_instance) { return t_instance.export_marked; };
  std::size_t num_export_marked_instances = std::count_if(all_instances.begin(), all_instances.end(), is_marked);

  if (num_export_marked_instances > 1) {
    throw std::runtime_error("More than one export-marked instance found");
  }

  if (num_export_marked_instances == 1) {
    return *std::find_if(all_instances.begin(), all_instances.end(), is_marked);
  }

  if (all_instances.size() > 1) {
    throw std::runtime_error("More than one instance found. Use export marking for disambiguation");
  }

  if (all_instances.empty()) {
    throw std::runtime_error("No instance found"); 
  }

  return all_instances[0];
}

std::string DebugString(const FMMatrix44& m) {
  std::stringstream ss;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      ss << '\t' << m[row][col];
    }
    ss << '\n';
  }
  return ss.str();
}

FCDGeometryPolygons* PolysFromGeometry(FCDGeometry* geom) {
  if (!geom->IsMesh()) {
    throw std::runtime_error("Non-mesh geometry");
  }

  FCDGeometryMesh* mesh = geom->GetMesh();
  if (!mesh->IsTriangles()) {
    FCDGeometryPolygonsTools::Triangulate(mesh);
  }

  if (mesh->GetPolygonsCount() != 1) {
    throw std::runtime_error("Mesh has != 1 set of polygons");
  }

  FCDGeometryPolygons* polys = mesh->GetPolygons(0);
  if (!polys->FindInput(FUDaeGeometryInput::POSITION)) {
    throw std::runtime_error("Missing positions");
  }

  if (!polys->FindInput(FUDaeGeometryInput::NORMAL)) {
    throw std::runtime_error("Missing normals");
  }

  if (!polys->FindInput(FUDaeGeometryInput::TEXCOORD)) {
    throw std::runtime_error("Missing texture coordinates");
  }

  // Generate tangent basis for normal mapping. We don't need
  // bitangent because that's generated at run time in vertex
  // shader.
  FCDGeometrySourceList tex_sources;
  polys->GetParent()->FindSourcesByType(FUDaeGeometryInput::TEXCOORD, tex_sources);
  FCDGeometryPolygonsTools::GenerateTextureTangentBasis(
      mesh, tex_sources[0], /*generateBinormals=*/false);

  if (!polys->FindInput(FUDaeGeometryInput::TEXTANGENT)) {
    throw std::runtime_error("Missing texture tangents");
  }

  return polys;
}

// Read all data from the input, looking up using indices.
std::vector<float> GetData(FCDGeometryPolygonsInput* input,
                           FCDGeometrySource* source = nullptr) {
  uint64_t num_vertices = input->GetIndexCount();
  const auto* indices = input->GetIndices();

  if (!source) {
    source = input->GetSource();
  }

  const float* source_data = source->GetData();
  int stride = source->GetStride();

  std::vector<float> data;
  data.reserve(num_vertices * stride);

  for (uint32_t i = 0; i < num_vertices; ++i) {
    uint32_t index = indices[i];
    for (int j = 0; j < stride; ++j) {
      data.push_back(source_data[index * stride + j]);
    }
  }

  return data;
}

struct VertexData {
  // 3x position, 3x normal, 3x tangent, 2x UV, 2x ambient occlusion UV, kMaxSkinInfluences bone ids, kMaxSkinInfluences bone weights.
  // We need to store everything in one big array because we need to be able to sort them efficiently for deduplication.
  // bone IDs should fit in the integer parts of float.
  float data[13 + kMaxSkinInfluences * 2];

  // Convenience functions.
  VertexData() { std::fill(std::begin(data), std::end(data), 0.0f); }
  float* Position() { return &data[0]; }
  float* Normal() { return &data[3]; }
  float* Tangent() { return &data[6]; }
  float* UV0() { return &data[9]; }
  float* UV1() { return &data[11]; }
  float* BoneId(int i) { return &data[13 + i]; }
  float* BoneWeight(int i) { return &data[13 + kMaxSkinInfluences + i]; }

  bool operator<(const VertexData& other) const {
    return std::lexicographical_compare(std::begin(data), std::end(data),
                                        std::begin(other.data), std::end(other.data));
  }

  bool operator==(const VertexData& other) const {
    return std::equal(std::begin(data), std::end(data), std::begin(other.data),
                      [](float a, float b) { return fabs(a - b) < 0.0000001f; });
  }

  bool operator!=(const VertexData& other) const {
    return !(*this == other);
  }

  void SetPosition(float* x) { std::copy(x, x + 3, Position()); }
  void SetNormal(float* x) { std::copy(x, x + 3, Normal()); }
  void SetTangent(float* x) { std::copy(x, x + 3, Tangent()); }
  void SetUV0(float* x) { std::copy(x, x + 2, UV0()); }
  void SetUV1(float* x) { std::copy(x, x + 2, UV1()); }
  void SetBoneId(int i, uint8_t id) { *(BoneId(i)) = static_cast<float>(id); }
  void SetBoneWeight(int i, float weight) { *(BoneWeight(i)) = weight; }

  void ApplyTransform(const FMMatrix44& model_matrix) {
    FMVector3 position(Position()[0], Position()[1], Position()[2]);
    FMVector3 normal(Normal()[0], Normal()[1], Normal()[2]);
    FMVector3 tangent(Tangent()[0], Tangent()[1], Tangent()[2]);

    // We should technically be using normal matrix here instead of model
    // matrix, but they are the same because we are not doing anisotropic
    // scaling.
    position = model_matrix.TransformVector(position);
    normal = model_matrix.TransformVector(normal);
    tangent = model_matrix.TransformVector(tangent);
    normal.NormalizeIt();
    tangent.NormalizeIt();
    SetPosition(position);
    SetNormal(normal);
    SetTangent(tangent);
  }
};

struct IndexedVertexData {
  std::vector<VertexData> vds;
  std::vector<uint32_t> indices;
};

struct RawBoneTransform {
	float translation[3];
	float orientation[4];
};

// Find duplicates, and switch to an indexed representation after de-duplication.
IndexedVertexData Reindex(std::vector<VertexData>&& vds) {
  // First sort lexicographically.
  using VdsWithIndex = std::pair<VertexData, uint32_t>;
  std::vector<VdsWithIndex> vds_with_indices;
  for (std::size_t i = 0; i < vds.size(); ++i) {
    vds_with_indices.push_back(VdsWithIndex(std::move(vds[i]), i));
  }
  std::sort(vds_with_indices.begin(), vds_with_indices.end(), [](const auto& a, const auto& b) {
    return a.first < b.first;
  });

  IndexedVertexData ret;

  // Mapping from original indices to new sorted and de-dupped indices.
  std::map<uint32_t, uint32_t> new_indices;

  // Go through the vertices looking for duplicates (only have to check previous one).
  for (std::size_t i = 0; i < vds_with_indices.size(); ++i) {
    if (ret.vds.empty() || (vds_with_indices[i].first != ret.vds.back())) {
      ret.vds.push_back(vds_with_indices[i].first);
    }
    new_indices[vds_with_indices[i].second] = (ret.vds.size() - 1);
  }

  for (std::size_t i = 0; i < vds_with_indices.size(); ++i) {
    ret.indices.push_back(new_indices[i]);
  }

  return ret;
}

// Reduce number of influences on each joint to kMaxSkinInfluences.
// See https://github.com/0ad/0ad/blob/c7d07d3979f969b969211a5e5748fa775f6768a7/source/collada/CommonConvert.cpp#L303
// We don't drop weights less than min weight because we don't want to branch in the shader anyways.
void ReduceInfluences(FCDSkinController* skin) {
  // For each vertex with influences
  for (std::size_t influence = 0; influence < skin->GetInfluenceCount(); ++influence) {
    FCDSkinControllerVertex* influences = skin->GetVertexInfluence(influence);
    std::vector<FCDJointWeightPair> new_weights;
    for (std::size_t i = 0; i < influences->GetPairCount(); ++i) {
      FCDJointWeightPair* weight = influences->GetPair(i);
      // See if there is an existing weight with the same joint. Merge if so.
      bool merged = false;
      for (std::size_t j = 0; j < new_weights.size(); ++j) {
        if (weight->jointIndex == new_weights[j].jointIndex) {
          new_weights[j].weight += weight->weight;
          merged = true;
          break;
        }
      }
      if (!merged) {
        new_weights.push_back(*weight);
      }
    }

    // Sort influences on the vertex by weight, and drop extra ones.
    std::sort(new_weights.begin(), new_weights.end(), [](const auto& a, const auto& b) {
      return a.weight > b.weight;
    });

    if (new_weights.size() > kMaxSkinInfluences) {
      new_weights.resize(kMaxSkinInfluences);
    }

    // Re-normalize.
    float total_weight = std::accumulate(
        new_weights.begin(), new_weights.end(), 0.0f,
        [](float x, const auto& weight) { return x + weight.weight; });
    if (total_weight > 0.0f) {
      for (auto& weight : new_weights) {
        weight.weight /= total_weight;
      }
    }

    influences->SetPairCount(0);
    for (const auto& weight : new_weights) {
      influences->AddPair(weight.jointIndex, weight.weight);
    }
  }

  skin->SetDirtyFlag();
}

std::vector<std::vector<uint8_t>> g_skeleton_buffers;
std::vector<const data::Skeleton*> g_skeletons;

class SkeletonSelection {
 public:
  const data::Skeleton* skeleton;
  const data::SkeletonMapping* mapping;

  SkeletonSelection() : skeleton(nullptr), mapping(nullptr) {}

  SkeletonSelection(const data::Skeleton* skeleton, const data::SkeletonMapping* mapping)
      : skeleton(skeleton), mapping(mapping) {
        name_to_id_ = std::map<std::string, int>();
    for (std::size_t i = 0; i < mapping->bone_names()->size(); ++i) {
      (*name_to_id_)[mapping->bone_names()->GetAsString(i)->str()] = mapping->canonical_ids()->Get(i);
    }
  }

  std::optional<int> FindBoneId(const std::string& s) const {
    const auto it = name_to_id_->find(s);
    if (it == name_to_id_->end()) {
      return std::nullopt;
    } else {
      return it->second;
    }
  }

  std::size_t GetBoneCount() const {
    if (name_to_id_) {
      return name_to_id_->size();
    } else {
      return 0;
    }
  }

 private:
  std::optional<std::map<std::string, int>> name_to_id_;
};

std::optional<SkeletonSelection> FindSkeleton(const std::string& root_name) {
  for (const auto* skeleton : g_skeletons) {
    for (const auto* mapping : *(skeleton->mappings())) {
      if (root_name == mapping->root_name()->str()) {
        return SkeletonSelection(skeleton, mapping);
      }
    }
  }
  return std::nullopt;
}

// See https://github.com/0ad/0ad/blob/c7d07d3979f969b969211a5e5748fa775f6768a7/source/collada/CommonConvert.cpp#L381
std::optional<SkeletonSelection> FindSkeleton(const FCDControllerInstance* controller_instance) {
  const FCDSceneNode* joint = controller_instance->GetJoint(0);
  while (joint) {
    auto maybe_selection = FindSkeleton(joint->GetName().c_str());
    if (maybe_selection) {
      return maybe_selection;
    }
    joint = joint->GetParent();
  }
  return std::nullopt;
}

struct AttachmentPoint {
  std::string name;
  FMMatrix44 transform;

  // 0xFF means no bone (relative to bind pose).
  uint8_t bone;
};

void AddStaticAttachmentPoints(FCDSceneNode* node, std::vector<AttachmentPoint>* attachment_points) {
  if (node->GetName().find("prop-") == 0 || node->GetName().find("prop_") == 0) {
    AttachmentPoint pt;
    pt.name = node->GetName().substr(5);
    LOG_DEBUG("Prop point % found", pt.name);
    pt.transform = node->CalculateWorldTransform();
    pt.bone = 0xFF;
    attachment_points->push_back(std::move(pt));
  }

  for (std::size_t i = 0; i < node->GetChildrenCount(); ++i) {
    AddStaticAttachmentPoints(node->GetChild(i), attachment_points);
  }
}

void AddSkinnedAttachmentPoints(FCDControllerInstance* controller_instance, std::size_t joint_count,
                                const SkeletonSelection& skeleton,
                                std::vector<AttachmentPoint>* attachment_points) {
  for (std::size_t i = 0; i < joint_count; ++i) {
    FCDSceneNode* joint = controller_instance->GetJoint(i);
    auto maybe_bone_id = skeleton.FindBoneId(joint->GetName().c_str());
    if (!maybe_bone_id) {
      // unrecognised joint name - ignore, same as before
      continue;
    }

    // Check all the objects attached to this bone
    for (size_t j = 0; j < joint->GetChildrenCount(); ++j) {
      FCDSceneNode* child = joint->GetChild(j);
      if (child->GetName().find("prop-") == 0 || child->GetName().find("prop_") == 0) {
        AttachmentPoint pt;
        pt.name = child->GetName().substr(5);
        LOG_DEBUG("Adding skinned prop point %", pt.name);
        pt.transform = child->ToMatrix();
        pt.bone = *maybe_bone_id;
        attachment_points->push_back(std::move(pt));
      }
    }
  }
}

void WriteImageOpt(const std::string& output_path, std::vector<uint8_t>* uncompressed, int width, int height) {
  // Make sure min alpha is 1, because libimagequant assumes we don't care about RGB if alpha = 0, and in textures
  // alpha may not actually be used as transparency.
  // https://github.com/ImageOptim/libimagequant/issues/45
  for (int i = 0; i < (width * height); ++i) {
    uint8_t* alpha = &((*uncompressed)[i * 4 + 3]);
    if (*alpha < 1) {
      *alpha = 1;
    }
  }

  liq_attr* handle = liq_attr_create();
  liq_image* input_image = liq_image_create_rgba(handle, uncompressed->data(), width, height, 0);
  liq_result* quantization_result;
  if (liq_image_quantize(input_image, handle, &quantization_result) != LIQ_OK) {
    LOG_ERROR("Failed to quantize %", output_path);
    return;
  }

  std::vector<uint8_t> raw_8bit_pixels(width * height);
  liq_set_dithering_level(quantization_result, 0.0);
  liq_set_quality(handle, 0, 80);
  liq_write_remapped_image(quantization_result, input_image, &raw_8bit_pixels[0], width * height);

  const liq_palette* palette = liq_get_palette(quantization_result);

  lodepng::State state;
  state.info_raw.colortype = LCT_PALETTE;
  state.info_raw.bitdepth = 8;
  state.info_png.color.colortype = LCT_PALETTE;
  state.info_png.color.bitdepth = 8;

  for (std::size_t i = 0; i < palette->count; ++i) {
    lodepng_palette_add(&state.info_png.color, palette->entries[i].r, palette->entries[i].g, palette->entries[i].b, palette->entries[i].a);
    lodepng_palette_add(&state.info_raw, palette->entries[i].r, palette->entries[i].g, palette->entries[i].b, palette->entries[i].a);
  }

  std::vector<uint8_t> png_data;
  uint32_t error = lodepng::encode(
      png_data, reinterpret_cast<const uint8_t*>(raw_8bit_pixels.data()), width, height, state);
  if (error) {
    LOG_ERROR("Failed to encode PNG: %", lodepng_error_text(error));
    return;
  }
  WriteToFile(output_path, png_data.data(), png_data.size());

  liq_result_destroy(quantization_result);
  liq_image_destroy(input_image);
  liq_attr_destroy(handle);
}

std::string GetTextContent(XMLHandle handle) {
  for (const XMLNode* child = handle.FirstChild().ToNode(); child != nullptr; child = child->NextSibling()) {
    const XMLText* text = child->ToText();
    if (text) {
      return text->Value();
    }
  }
  return "";
}

}

// See https://github.com/0ad/0ad/blob/master/source/collada/PMDConvert.cpp
// We try to match the logic in 0ad as much as possible because these assets
// have been tested with 0ad.
void ParseMesh(const std::string& mesh_path) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(mesh_path)) {
    return;
  }

  std::string full_path = std::string(kInputPrefix) + kMeshPathPrefix + mesh_path;
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing mesh % at %", mesh_path, full_path);
  std::string source = ReadWholeFileString(full_path);
  ColladaDocument cdoc;

  cdoc.LoadFromText(source);

  FCDSceneNode* root = cdoc.Doc()->GetVisualSceneRoot();
  if (!root) {
    LOG_ERROR("Failed to parse %: we have no root", mesh_path);
    return;
  }

  FMVector3 up_axis = cdoc.Doc()->GetAsset()->GetUpAxis();
  bool yup = (up_axis.y != 0); // assume either Y_UP or Z_UP.

  TransformedInstance t_instance = FindSingleInstance(root);

  FCDGeometryPolygons* polys = nullptr;

  FCDSkinController* skin = nullptr;
  FCDControllerInstance* controller_instance = nullptr;

  std::size_t joint_count = 0;

  SkeletonSelection skeleton;

  if (t_instance.instance->GetEntity()->GetType() == FCDEntity::GEOMETRY) {
    LOG_DEBUG("Found static geometry");
    polys = PolysFromGeometry(static_cast<FCDGeometry*>(t_instance.instance->GetEntity()));
  } else if (t_instance.instance->GetEntity()->GetType() == FCDEntity::CONTROLLER) {
    LOG_DEBUG("Found skinned geometry");
    controller_instance = static_cast<FCDControllerInstance*>(t_instance.instance);
    FCDController* controller = static_cast<FCDController*>(t_instance.instance->GetEntity());
    skin = controller->GetSkinController();
    if (!skin) {
      LOG_ERROR("No skin controller on skinned geometry?");
      return;
    }
    auto maybe_skeleton = FindSkeleton(controller_instance);

    if (!maybe_skeleton) {
      LOG_ERROR("No skeleton found");
      return;
    }

    skeleton = *maybe_skeleton;

    joint_count = std::min(skin->GetJointCount(), controller_instance->GetJointCount());
    if (skin->GetJointCount() != controller_instance->GetJointCount()) {
      LOG_ERROR("Skin and controller have different joint counts: % vs %",
                skin->GetJointCount(), controller_instance->GetJointCount());
      return;
    }
    ReduceInfluences(skin);
    polys = PolysFromGeometry(controller->GetBaseGeometry());
  } else {
    LOG_ERROR("Unknown geometry type: %", t_instance.instance->GetEntity()->GetType());
    return;
  }

  auto num_vertices = polys->GetFaceVertexCount();

  LOG_DEBUG("% vertices", num_vertices);

  auto positions_data = GetData(polys->FindInput(FUDaeGeometryInput::POSITION));
  auto normals_data = GetData(polys->FindInput(FUDaeGeometryInput::NORMAL));
  auto tangents_data = GetData(polys->FindInput(FUDaeGeometryInput::TEXTANGENT));

  std::vector<std::vector<float>> texcoords_data;
  FCDGeometrySourceList tex_sources;
  polys->GetParent()->FindSourcesByType(FUDaeGeometryInput::TEXCOORD, tex_sources);
  for (auto& tex_source : tex_sources) {
    texcoords_data.push_back(GetData(polys->FindInput(FUDaeGeometryInput::TEXCOORD),
                                     tex_source));
  }

  if (positions_data.size() != num_vertices * 3) {
    LOG_ERROR("Wrong position array size: % (expected %)", positions_data.size(), num_vertices * 3);
    return;
  }

  if (normals_data.size() != num_vertices * 3) {
    LOG_ERROR("Wrong normal array size: % (expected %)", normals_data.size(), num_vertices * 3);
    return;
  }

  if (tangents_data.size() != num_vertices * 3) {
    LOG_ERROR("Wrong tangent array size: % (expected %)", tangents_data.size(), num_vertices * 3);
    return;
  }

  if (texcoords_data.empty()) {
    LOG_ERROR("No texcoords");
    return;
  }

  if (texcoords_data.size() > 2) {
    LOG_ERROR("We have % texcoords. What do we do with the extra ones?", texcoords_data.size());
    return;
  }

  auto* position_source = polys->FindInput(FUDaeGeometryInput::POSITION)->GetSource();
  auto num_vertex_positions = position_source->GetDataCount() / position_source->GetStride();
  if (skin && skin->GetInfluenceCount() != num_vertex_positions) {
    LOG_ERROR("Influence count (%) != (pre-indexed) vertices count (%)", skin->GetInfluenceCount(), num_vertex_positions);
    return;
  }

  LOG_DEBUG("% texture coordinate sets", texcoords_data.size());

  std::vector<VertexData> vertex_datas(num_vertices);

  auto* positions_indices = polys->FindInput(FUDaeGeometryInput::POSITION)->GetIndices();

  constexpr RawBoneTransform kBoneDefault = {{0, 0, 0}, {0, 0, 0, 1}};
  std::vector<RawBoneTransform> bind_pose_bones(skeleton.GetBoneCount(), kBoneDefault);

  // We use z-up, so do a rotate if the model is y-up.
  FMMatrix44 up_transform = FMMatrix44::Identity;
  if (yup) {
    up_transform = FMMatrix44::XAxisRotationMatrix(M_PI / 2.0f);
  }

  // This is either entity transform if not skinned, or skin->GetBindShapeTransform() if
  // skinned. The static case is equivalent to converter.GetEntityTransform() in 0ad.
  FMMatrix44 model_transform = skin ? skin->GetBindShapeTransform() : t_instance.transform;

  for (uint64_t vertex = 0; vertex < num_vertices; ++vertex) {
    VertexData* vd = &vertex_datas[vertex];
    vd->SetPosition(&positions_data[vertex * 3]);
    vd->SetNormal(&normals_data[vertex * 3]);
    vd->SetTangent(&tangents_data[vertex * 3]);
    vd->SetUV0(&texcoords_data[0][vertex * 2]);

    if (texcoords_data.size() >= 2) {
      vd->SetUV1(&texcoords_data[1][vertex * 2]);
    }

    if (skin) {
      FCDSkinControllerVertex* vertex_influences = skin->GetVertexInfluence(positions_indices[vertex]);
      std::vector<std::pair<int32_t, float>> influences;
      for (std::size_t i = 0; i < vertex_influences->GetPairCount(); ++i) {
        auto* inf = vertex_influences->GetPair(i);
        influences.push_back(std::make_pair(inf->jointIndex, inf->weight));
      }

      for (auto& [bone, weight] : influences) {
        // If bone is -1, we change it to the virtual bind pose bone above. See:
        // https://github.com/0ad/0ad/blob/aee0a07666750f6aa0f56561e8113aac43df813e/source/collada/PMDConvert.cpp#L276
        if (bone == -1) {
          bone = joint_count;
        } else {
          if (bone > 0xFE) {
            LOG_ERROR("Bone % is above max 254", bone);
            throw std::runtime_error("Bone out of bound");
          }

          // Remap the bone index with skeleton lookup.
          FCDSceneNode* joint = nullptr;
          if (static_cast<std::size_t>(bone) < controller_instance->GetJointCount()) {
            joint = controller_instance->GetJoint(bone);
          }

          if (!joint) {
            LOG_ERROR("Vertex refers to non-existent joint idx %, max %", bone, (controller_instance->GetJointCount() - 1));
            throw std::runtime_error("Non-existent joint");
          }

          auto maybe_bone_id = skeleton.FindBoneId(joint->GetName().c_str());
          if (!maybe_bone_id) {
            LOG_ERROR("Bone % not found in skeleton %", joint->GetName().c_str(), skeleton.skeleton->id());
            throw std::runtime_error("Non-existent joint");
          }

          bone = *maybe_bone_id;
        }
      }

      // If we have no influences, add one that just drives the vertex to bind pose - a "virtual bone" at
      // end of the joints list (index joint_count).
      if (influences.empty()) {
        influences.push_back(std::make_pair(joint_count, 1.0f));
      }

      // Pad to max skin influences.
      while (influences.size() < kMaxSkinInfluences) {
        influences.push_back(std::make_pair(0xFF, 0.0f));
      }

      for (std::size_t i = 0; i < influences.size(); ++i) {
        const auto& [bone, weight] = influences[i];
        vd->SetBoneId(i, bone);
        vd->SetBoneWeight(i, weight);
      }

      // Now we need to convert bind poses to bone transforms (so the renderer can add it as the "virtual bone"
      // while rendering this mesh.
			for (size_t i = 0; i < joint_count; ++i) {
				FCDSceneNode* joint = controller_instance->GetJoint(i);

				auto maybe_bone_id = skeleton.FindBoneId(joint->GetName().c_str());
				if (!maybe_bone_id) {
					// unrecognised joint - it's probably just a prop point
					// or something, so ignore it
					continue;
				}

				FMMatrix44 bind_pose = skin->GetJoint(i)->GetBindPoseInverse().Inverted();

				HMatrix matrix;
				memcpy(matrix, bind_pose.Transposed().m, sizeof(matrix));
					// set matrix = bindPose^T, to match what decomp_affine wants

				AffineParts parts;
				decomp_affine(matrix, &parts);

				RawBoneTransform b = {
					{ parts.t.x, parts.t.y, parts.t.z },
					{ parts.q.x, parts.q.y, parts.q.z, parts.q.w }
				};

				bind_pose_bones[*maybe_bone_id] = b;
			}
    }
  }

  IndexedVertexData ivd = Reindex(std::move(vertex_datas));

  LOG_DEBUG("% deduplicated vertex data", ivd.vds.size());

  std::vector<float> vertices(ivd.vds.size() * 3);
  for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
    float* vec = ivd.vds[i].Position();
    vertices[i * 3] = vec[0];
    vertices[i * 3 + 1] = vec[1];
    vertices[i * 3 + 2] = vec[2];
  }

  std::vector<float> normals(ivd.vds.size() * 3);
  for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
    float* vec = ivd.vds[i].Normal();
    normals[i * 3] = vec[0];
    normals[i * 3 + 1] = vec[1];
    normals[i * 3 + 2] = vec[2];
  }

  std::vector<float> tangents(ivd.vds.size() * 3);
  for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
    float* vec = ivd.vds[i].Tangent();
    tangents[i * 3] = vec[0];
    tangents[i * 3 + 1] = vec[1];
    tangents[i * 3 + 2] = vec[2];
  }

  std::vector<float> tex_coords(ivd.vds.size() * 2);
  for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
    float* vec = ivd.vds[i].UV0();
    tex_coords[i * 2] = vec[0];
    tex_coords[i * 2 + 1] = -vec[1];
  }

  std::vector<float> ao_tex_coords(ivd.vds.size() * 2, 0.0f);
  if (texcoords_data.size() >= 2) {
    for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
      float* vec = ivd.vds[i].UV1();
      ao_tex_coords[i * 2] = vec[0];
      ao_tex_coords[i * 2 + 1] = -vec[1];
    }
  }

  std::vector<uint8_t> bone_indices(ivd.vds.size() * kMaxSkinInfluences);
  if (skin) {
    for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
      for (int bone = 0; bone < kMaxSkinInfluences; ++bone) {
        bone_indices[i * kMaxSkinInfluences + bone] = static_cast<uint8_t>(*ivd.vds[i].BoneId(bone));
      }
    }
  }

  std::vector<float> bone_weights(ivd.vds.size() * kMaxSkinInfluences);
  if (skin) {
    for (uint32_t i = 0; i < ivd.vds.size(); ++i) {
      for (int bone = 0; bone < kMaxSkinInfluences; ++bone) {
        bone_weights[i * kMaxSkinInfluences + bone] = *ivd.vds[i].BoneWeight(bone);
      }
    }
  }

  // bind_pose_transform is size 0 if not skinned.
  std::vector<float> bind_pose_transforms;
  if (skin) {
    for (const auto& transform : bind_pose_bones) {
      std::copy(transform.translation, transform.translation + 3, std::back_inserter(bind_pose_transforms));
      std::copy(transform.orientation, transform.orientation + 4, std::back_inserter(bind_pose_transforms));
    }
  }

  std::vector<AttachmentPoint> attachment_points;

  if (skin) {
    AddSkinnedAttachmentPoints(controller_instance, joint_count, skeleton, &attachment_points);
  } else {
    AddStaticAttachmentPoints(root, &attachment_points);
  }

  // Add the "root" attachment point, which is the DAE root, and where prop points, bind poses, etc, are
  // all relative to when we use Collada to calculate world transform.
  AttachmentPoint root_pt;
  root_pt.name = "root";
  root_pt.transform = up_transform;
  attachment_points.push_back(std::move(root_pt));

  // Add The "mesh_root" attachment point, which is "root" plus entity/model transform.
  AttachmentPoint mesh_pt;
  mesh_pt.name = "mesh_root";
  mesh_pt.transform = up_transform * model_transform;
  attachment_points.push_back(std::move(mesh_pt));

  std::vector<std::string> attachment_point_names;
  std::vector<float> attachment_point_transforms;
  std::vector<uint8_t> attachment_point_bones;

  for (const auto& [name, matrix, bone] : attachment_points) {
    attachment_point_names.push_back(name);

    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        attachment_point_transforms.push_back(matrix[row][col]);
      }
    }

    attachment_point_bones.push_back(bone);
  }

  auto mesh_fb = data::CreateMesh(
    builder,
    /*path=*/builder.CreateString(RemoveExtension(mesh_path)),
    /*vertex_indices=*/builder.CreateVector(ivd.indices),
    /*vertices=*/builder.CreateVector(vertices),
    /*normals=*/builder.CreateVector(normals),
    /*tangents=*/builder.CreateVector(tangents),
    /*tex_coords=*/builder.CreateVector(tex_coords),
    /*ao_tex_coords=*/builder.CreateVector(ao_tex_coords),
    /*bone_indices=*/builder.CreateVector(bone_indices),
    /*bone_weights=*/builder.CreateVector(bone_weights),
    /*bind_pose_transforms=*/builder.CreateVector(bind_pose_transforms),
    /*attachment_point_names=*/builder.CreateVectorOfStrings(attachment_point_names),
    /*attachment_point_transforms=*/builder.CreateVector(attachment_point_transforms),
    /*attachment_point_bones=*/builder.CreateVector(attachment_point_bones)
    );
  builder.Finish(mesh_fb);
  WriteFB(std::string(kOutputPrefix) + kMeshPathPrefix + RemoveExtension(mesh_path),
          builder);
}

std::unique_ptr<ThreadPool> g_texture_animation_pool;

// Normal maps need to have inverted green for right handed coordinate system.
void SaveTexture(const std::string& texture_path, bool invert_green) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(texture_path)) {
    return;
  }
  std::string full_path = std::string(kInputPrefix) + kTexturePathPrefix + texture_path;
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  std::ifstream is(full_path);
  auto file_content = ReadWholeFile(full_path);
  std::string old_extension = Extension(texture_path);
  std::string output_path =
      std::string(kOutputPrefix) + kTexturePathPrefix + RemoveExtension(texture_path) + ".png";
  if (old_extension == "png" || old_extension == "dds") {
    // RGBA uncompressed data.
    std::vector<uint8_t> uncompressed;
    uint32_t width = 0;
    uint32_t height = 0;
    if (old_extension == "png") {
      // Re-encode and optimize PNG files.
      uint32_t error = lodepng::decode(uncompressed, width, height, file_content);
      if (error) {
        LOG_ERROR("Failed to decode PNG file %: %", full_path, lodepng_error_text(error));
        return;
      }
    } else if (old_extension == "dds") {
      gli::texture texture_load = gli::load(full_path);
      if (texture_load.empty()) {
        LOG_ERROR("Failed to load DDS texture: %", full_path);
        return;
      }
      if (texture_load.target() != gli::TARGET_2D) {
        LOG_ERROR("Unknown texture target: %", texture_load.target());
        return;
      }
      gli::texture2d texture = gli::texture2d(texture_load);
      gli::fsampler2D read_sampler(texture, gli::WRAP_CLAMP_TO_EDGE);
      for (int y = 0; y < texture.extent().y; ++y) {
        for (int x = 0; x < texture.extent().x; ++x) {
          glm::vec4 data = read_sampler.texel_fetch(gli::texture2d::extent_type(x, y), 0);
          for (int p = 0; p < 4; ++p) {
            uncompressed.push_back(data[p] * 255);
          }
        }
      }
      width = texture.extent().x;
      height = texture.extent().y;
    }

    if (invert_green) {
      for (uint32_t pixel = 0; pixel < (uncompressed.size() / 4); ++pixel) {
        auto idx = pixel * 4 + 1;
        uncompressed[idx] = 255 - uncompressed[idx];
      }
    }
    WriteImageOpt(output_path, &uncompressed, width, height);
  } else {
    LOG_ERROR("Unknown texture extension: %", old_extension);
    return;
  }
}

std::pair<float, float> GetAnimationRange(
  const FCDocument* doc, const SkeletonSelection& skeleton_selection,
	const FCDControllerInstance& controller_instance) {
  // FCollada tools export <extra> info in the scene to specify the start
  // and end times.
  // If that isn't available, we have to search for the earliest and latest
  // keyframes on any of the bones.
  if (doc->HasStartTime() && doc->HasEndTime()) {
    return std::make_pair(doc->GetStartTime(), doc->GetEndTime());
  }

  // XSI support omitted (there doesn't seem to be any XSI file in the assets anyways?)

  float start = std::numeric_limits<float>::max();
  float end = std::numeric_limits<float>::lowest();
  for (size_t i = 0; i < controller_instance.GetJointCount(); ++i)
  {
    const FCDSceneNode* joint = controller_instance.GetJoint(i);
    if (joint == nullptr) {
      throw std::runtime_error("Joint doesn't exist");
    }

    auto maybe_bone_id = skeleton_selection.FindBoneId(joint->GetName().c_str());

    if (!maybe_bone_id) {
      // unrecognised joint - it's probably just a prop point
      // or something, so ignore it
      continue;
    }

    // Skip unanimated joints
    if (joint->GetTransformCount() == 0) {
      continue;
    }

    for (std::size_t j = 0; j < joint->GetTransformCount(); ++j) {
      const FCDTransform* transform = joint->GetTransform(j);

      if (!transform->IsAnimated()) {
        continue;
      }

      // Iterate over all curves to find the earliest and latest keys
      const FCDAnimated* anim = transform->GetAnimated();
      const FCDAnimationCurveListList& curve_list = anim->GetCurves();
      for (size_t k = 0; k < curve_list.size(); ++k) {
        const FCDAnimationCurveTrackList& curves = curve_list[k];
        for (size_t l = 0; l < curves.size(); ++l) {
          const FCDAnimationCurve* curve = curves[l];
          start = std::min(start, curve->GetKeys()[0]->input);
          end = std::max(end, curve->GetKeys()[curve->GetKeyCount()-1]->input);
        }
      }
    }
  }

  return std::make_pair(start, end);
}

void EvaluateAnimations(FCDSceneNode& node, float time)	{
  for (size_t i = 0; i < node.GetTransformCount(); ++i)
  {
    FCDTransform* transform = node.GetTransform(i);
    FCDAnimated* anim = transform->GetAnimated();
    if (anim)
      anim->Evaluate(time);
  }

  for (size_t i = 0; i < node.GetChildrenCount(); ++i)
    EvaluateAnimations(*node.GetChild(i), time);
}

// The logic here is copied from:
// https://github.com/0ad/0ad/blob/aee0a07666750f6aa0f56561e8113aac43df813e/source/collada/PSAConvert.cpp#L61
void SaveAnimation(const std::string& animation_path) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(animation_path)) {
    return;
  }
  std::string full_path = std::string(kInputPrefix) + kAnimationPathPrefix + animation_path;
  LOG_INFO("Saving animation: %", animation_path);
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  std::ifstream is(full_path);
  auto file_content = ReadWholeFileString(full_path);
  std::string output_path =
      std::string(kOutputPrefix) + kAnimationPathPrefix + RemoveExtension(animation_path);
  ColladaDocument cdoc;
  cdoc.LoadFromText(file_content);

  FCDSceneNode* root = cdoc.Doc()->GetVisualSceneRoot();
  if (!root) {
    LOG_ERROR("Failed to parse %: we have no root", animation_path);
    return;
  }

  TransformedInstance t_instance = FindSingleInstance(root);
  if (t_instance.instance->GetEntity()->GetType() == FCDEntity::CONTROLLER) {
    FCDControllerInstance& controller_instance =
        static_cast<FCDControllerInstance&>(*(t_instance.instance));

    FCDController* controller =
        static_cast<FCDController*>(t_instance.instance->GetEntity());

    FCDSkinController* skin = controller->GetSkinController();
    if (skin == NULL) {
      throw std::runtime_error("Not a skin controller?");
    }

    auto maybe_skeleton = FindSkeleton(&controller_instance);

    if (!maybe_skeleton) {
      LOG_ERROR("No skeleton found: %", full_path);
      throw std::runtime_error("No skeleton found");
    }

    SkeletonSelection skeleton_selection = *maybe_skeleton;

    constexpr float kFrameLength = 1.0f / 30.0f; // Always 30fps for now.

    // Find the extents of the animation:
    auto [time_start, time_end] = GetAnimationRange(
        cdoc.Doc(), skeleton_selection, controller_instance);
    // To catch broken animations / skeletons.xml:
    if (time_end <= time_start) {
      LOG_ERROR("animation end frame must come after start frame: %", full_path);
      throw std::runtime_error("animation end frame must come after start frame");
    }

    // Count frames; don't include the last keyframe
    size_t frame_count = static_cast<std::size_t>((time_end - time_start) / kFrameLength - 0.5f);
    if (frame_count == 0) {
      LOG_ERROR("Animation must have frames: %", full_path);
      throw std::runtime_error("animation must have frames");
    }

    const size_t bone_count = skeleton_selection.mapping->bone_names()->size();
    if (bone_count > 64) {
      LOG_ERROR("Skeleton has too many bones %/64", bone_count);
      throw std::runtime_error("Skeleton has too many bones");
    }

    std::vector<RawBoneTransform> bone_transforms;

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
      float time = time_start + kFrameLength * frame;

      constexpr RawBoneTransform kBoneDefault = {{0, 0, 0}, {0, 0, 0, 1}};
      std::vector<RawBoneTransform> frame_bone_transforms(bone_count, kBoneDefault);

      // Move the model into the new animated pose
      // (We can't tell exactly which nodes should be animated, so
      // just update the entire world recursively)
      EvaluateAnimations(*root, time);

      // Convert the pose into the form require by the game
      for (size_t i = 0; i < controller_instance.GetJointCount(); ++i) {
        FCDSceneNode* joint = controller_instance.GetJoint(i);

        auto maybe_bone_id = skeleton_selection.FindBoneId(joint->GetName().c_str());
        if (!maybe_bone_id) {
          continue;  // not a recognised bone - ignore it, same as before
        }

        FMMatrix44 world_transform = joint->CalculateWorldTransform();

        HMatrix matrix;
        memcpy(matrix, world_transform.Transposed().m, sizeof(matrix));

        AffineParts parts;
        decomp_affine(matrix, &parts);

        RawBoneTransform b = {{parts.t.x, parts.t.y, parts.t.z},
                           {parts.q.x, parts.q.y, parts.q.z, parts.q.w}};

        frame_bone_transforms[*maybe_bone_id] = b;
      }

      // Push frameRawBoneTransforms onto the back of RawBoneTransforms
      std::copy(frame_bone_transforms.begin(), frame_bone_transforms.end(),
                std::back_inserter(bone_transforms));
    }

    std::vector<float> bone_states_buf;
    bone_states_buf.reserve(bone_count * frame_count * 7);
    for (const auto& transform : bone_transforms) {
      std::copy(transform.translation, transform.translation + 3,
                std::back_inserter(bone_states_buf));
      std::copy(transform.orientation, transform.orientation + 4,
                std::back_inserter(bone_states_buf));
    }

    flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
    auto animation = data::CreateAnimation(
      builder,
      /*path=*/builder.CreateString(RemoveExtension(animation_path)),
      /*frame_time=*/kFrameLength,
      /*num_bones=*/bone_count,
      /*num_frames=*/frame_count,
      /*bone_states=*/builder.CreateVector(bone_states_buf)
    );
    builder.Finish(animation);    
    WriteFB(output_path, builder);
  } else {
    throw std::runtime_error(std::string("Unrecognised entity type in: ") + full_path);
  }
}

void EnqueueTexture(const std::string& texture_path, bool invert_green) {
  g_texture_animation_pool->Push([=]() { SaveTexture(texture_path, invert_green); });
  LOG_INFO("Enqueued texture % (invert %)", texture_path, invert_green);
}

void EnqueueAnimation(const std::string& animation_path) {
  g_texture_animation_pool->Push([animation_path]() { SaveAnimation(animation_path); });
  LOG_INFO("Enqueued animation %", animation_path);
}

std::unique_ptr<ThreadPool> g_parser_pool;

void MakeActor(const std::string& actor_path) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(actor_path)) {
    return;
  }
  std::string full_path = std::string(kInputPrefix) + kActorPathPrefix + RemoveExtension(actor_path) + ".xml";
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing actor % at %", actor_path, full_path);
  TinyXMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open: ") + full_path);
  }

  XMLHandle root(nullptr);

  // An actor may have multiple qualitylevels. Get the highest quality one if so.
  auto* qualitylevels = xml_doc.FirstChildElement("qualitylevels");
  XMLElement* maybe_inline = nullptr;
  if (qualitylevels != nullptr) {
    std::vector<XMLHandle> actor_levels = GetAllChildrenElements(XMLHandle(qualitylevels), "actor");
    for (auto& actor : actor_levels) {
      const char* maybe_quality = actor.ToElement()->Attribute("quality");
      // We are looking for one with either quality="high" or no quality attribute.
      if (!maybe_quality || std::string(maybe_quality) == "high") {
        root = actor;
        break;
      }
    }
    if (!root.ToElement()) {
      throw std::runtime_error(std::string("Didn't find right quality level actor: ") + full_path);
    }

    // TODO: what do inline versions mean and do we care about them?
    maybe_inline = qualitylevels->FirstChildElement("inline");
  } else {
    root = XMLHandle(xml_doc.FirstChildElement("actor"));
  }

  if (maybe_inline != nullptr && root.ToElement()->Attribute("inline") != nullptr) {
    XMLNode* child = maybe_inline->FirstChild();
    while (child) {
      root.ToElement()->InsertEndChild(DeepCopy(child, &xml_doc));
      child = child->NextSibling();
    }
  }

  std::vector<XMLHandle> xml_groups = GetAllChildrenElements(root, "group");

  if (xml_groups.empty()) {
    throw std::runtime_error(full_path + " has no groups?");
  }

  std::string material;

  std::vector<XMLHandle> materials = GetAllChildrenElements(root, "material");
  if (!materials.empty()) {
    material = GetTextContent(materials[0]);
    LOG_DEBUG("Material is: %", material);
  }

  LOG_DEBUG("% groups found", xml_groups.size());

  std::vector<flatbuffers::Offset<data::Group>> groups;

  for (auto xml_group : xml_groups) {
    // Each group should have 1 or more variant.
    auto xml_variants = GetAllChildrenElements(xml_group, "variant");
    if (xml_variants.empty()) {
      throw std::runtime_error(full_path + " has a group with no variant");
    }
    LOG_DEBUG("% variants found", xml_variants.size());

    std::vector<flatbuffers::Offset<data::Variant>> variants;

    for (XMLHandle xml_variant : xml_variants) {
      ProcessVariantIncludes(xml_variant.ToElement(), &xml_doc);

      float frequency = xml_variant.ToElement()->FloatAttribute("frequency", -1.0f);

      if (frequency == 0.0f) {
        // Skip old unused variants, but not variants without frequency (-1.0f default)
        // because they can be used by code when named.
        continue;
      }

      frequency = std::max(0.0f, frequency);

      std::string mesh_path;

      auto meshes = GetAllChildrenElements(xml_variant, "mesh");
      if (!meshes.empty()) {
        if (meshes.size() != 1) {
          throw std::runtime_error("More than one mesh in a variant?");
        }
        mesh_path = GetTextContent(meshes[0]);
        ParseMesh(mesh_path);
        mesh_path = RemoveExtension(mesh_path) + ".fb";
      }

      auto colors = GetAllChildrenElements(xml_variant, "color");
      std::optional<data::Colour> maybe_object_colour;
      if (!colors.empty()) {
        if (colors.size() != 1) {
          throw std::runtime_error("More than one color in a variant?");
        }
        std::stringstream ss(GetTextContent(colors[0]));
        float r, g, b;
        ss >> r >> g >> b;
        r /= 255.0f;
        g /= 255.0f;
        b /= 255.0f;
        maybe_object_colour = data::Colour(r, g, b);
      }

      auto textures_containers = GetAllChildrenElements(xml_variant, "textures");
      std::vector<flatbuffers::Offset<data::Texture>> texture_offsets;
      if (textures_containers.size() > 1) {
        throw std::runtime_error("More than one <texture>?");
      } else if (textures_containers.size() == 1) {
        auto textures = GetAllChildrenElements(textures_containers[0], "texture");
        for (auto& texture : textures) {
          std::string texture_name = texture.ToElement()->Attribute("name");
          std::string texture_file = texture.ToElement()->Attribute("file");
          bool invert_green = texture_name == "normTex";
          EnqueueTexture(kActorTexturePathPrefix + texture_file, invert_green);
          texture_offsets.push_back(data::CreateTexture(
              builder,
              /*name=*/builder.CreateString(texture_name),
              /*file=*/builder.CreateString(kActorTexturePathPrefix + RemoveExtension(texture_file))
            ));
        }
      }

      std::vector<flatbuffers::Offset<data::Prop>> props_offsets;
      auto props_containers = GetAllChildrenElements(xml_variant, "props");
      if (!props_containers.empty()) {
        auto props = GetAllChildrenElements(props_containers[0], "prop");
        for (auto& prop : props) {
          const char* actor_attr = prop.ToElement()->Attribute("actor");
          const char* attachpoint_attr = prop.ToElement()->Attribute("attachpoint");
          if (!attachpoint_attr) {
            throw std::runtime_error("Prop with no attachpoint?");
          }
          if (!actor_attr || std::string(actor_attr).empty()) {
            // A prop without actor means we are clearing an attachpoint.
            props_offsets.push_back(data::CreateProp(
                builder,
                /*actor=*/builder.CreateString(""),
                /*attachpoint=*/builder.CreateString(attachpoint_attr)));
            LOG_DEBUG("Prop: (detach) (%)", attachpoint_attr);
          } else {
            std::string actor = actor_attr;
            std::string attachpoint = attachpoint_attr;
            props_offsets.push_back(data::CreateProp(
                builder,
                /*actor=*/builder.CreateString(RemoveExtension(actor)),
                /*attachpoint=*/builder.CreateString(attachpoint)));
            LOG_DEBUG("Prop: % (%)", actor, attachpoint);
            MakeActor(actor);
          }
        }
      }

      auto animations_containers = GetAllChildrenElements(xml_variant, "animations");
      std::vector<flatbuffers::Offset<data::AnimationSpec>> animation_offsets;
      if (animations_containers.size() > 1) {
        throw std::runtime_error("More than one <animations>?");
      } else if (animations_containers.size() == 1) {
        auto animations = GetAllChildrenElements(animations_containers[0], "animation");
        for (auto& animation : animations) {
          auto name_attrib = animation.ToElement()->Attribute("name");
          auto file_attrib = animation.ToElement()->Attribute("file");
          std::string name = name_attrib ? name_attrib : "";
          std::string file = file_attrib ? file_attrib : "";
          int int_speed = animation.ToElement()->IntAttribute("speed", 0);
          // See https://github.com/0ad/0ad/blob/412f1d0da275a12df16e140fca7523590e5f7cfe/source/graphics/ObjectBase.cpp#L310
          float speed = int_speed > 0 ? (int_speed / 100.0f) : 1.0f;
          int frequency = animation.ToElement()->IntAttribute("frequency", 1);
          if (file != "") {
            EnqueueAnimation(file);
          }
          animation_offsets.push_back(data::CreateAnimationSpec(
              builder,
              /*path=*/builder.CreateString(RemoveExtension(file)),
              /*name=*/builder.CreateString(name),
              /*speed=*/speed,
              /*frequency=*/frequency)
          );
        }
      }

      std::string name;
      if (xml_variant.ToElement()->Attribute("name")) {
        name = xml_variant.ToElement()->Attribute("name");
      }

      auto name_offset = builder.CreateString(name);
      auto mesh_path_offset = builder.CreateString(mesh_path);
      auto props_vector_offset = builder.CreateVector(props_offsets);
      auto textures_vector_offset = builder.CreateVector(texture_offsets);
      auto animations_vector_offset = builder.CreateVector(animation_offsets);

      data::VariantBuilder variant_builder(builder);
      variant_builder.add_name(name_offset);
      variant_builder.add_frequency(frequency);
      variant_builder.add_mesh_path(mesh_path_offset);
      variant_builder.add_props(props_vector_offset);
      variant_builder.add_textures(textures_vector_offset);
      variant_builder.add_animations(animations_vector_offset);

      if (maybe_object_colour) {
        variant_builder.add_object_colour(&(*maybe_object_colour));
      }

      variants.push_back(variant_builder.Finish());
    }
    groups.push_back(CreateGroup(builder, /*variants=*/builder.CreateVector(variants)));
  }
  auto actor = data::CreateActor(
      builder,
      /*path=*/builder.CreateString(RemoveExtension(actor_path)),
      /*groups=*/builder.CreateVector(groups),
      /*material=*/builder.CreateString(material));
  builder.Finish(actor);
  WriteFB(std::string(kOutputPrefix) + kActorPathPrefix + RemoveExtension(actor_path),
          builder);
}

void MakeTerrain(const std::string& terrain_path) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(terrain_path)) {
    return;
  }
  std::string full_path = std::string(kInputPrefix) + kTerrainPathPrefix + terrain_path + ".xml";
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing terrain % at %", terrain_path, full_path);
  TinyXMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open: ") + full_path);
  }

  XMLHandle root = XMLHandle(xml_doc.FirstChildElement("terrain"));

  auto textures_containers = GetAllChildrenElements(root, "textures");
  std::vector<flatbuffers::Offset<data::Texture>> texture_offsets;
  if (textures_containers.size() > 1) {
    throw std::runtime_error("More than one <texture>?");
  } else if (textures_containers.size() == 1) {
    auto textures = GetAllChildrenElements(textures_containers[0], "texture");
    for (auto& texture : textures) {
      std::string texture_name = texture.ToElement()->Attribute("name");
      std::string texture_file = texture.ToElement()->Attribute("file");
      bool invert_green = texture_name == "normTex";
      EnqueueTexture(kTerrainTexturePathPrefix + texture_file, invert_green);
      texture_offsets.push_back(data::CreateTexture(
          builder,
          /*name=*/builder.CreateString(texture_name),
          /*file=*/builder.CreateString(kTerrainTexturePathPrefix + RemoveExtension(texture_file))
        ));
    }
  }

  auto terrain = data::CreateTerrain(
      builder,
      /*path=*/builder.CreateString(RemoveExtension(terrain_path)),
      /*textures=*/builder.CreateVector(texture_offsets));
  builder.Finish(terrain);
  WriteFB(std::string(kOutputPrefix) + kTerrainPathPrefix + RemoveExtension(terrain_path),
          builder);
}

void GetAllStandardSkeletonBones(XMLHandle node, std::vector<std::string>* bones) {
  auto children = GetAllChildrenElements(node, "bone");
  for (auto& child : children) {
    bones->push_back(child.ToElement()->Attribute("name"));
    GetAllStandardSkeletonBones(child, bones);
  }
}

// Here we try to figure out a mapping from skeleton bones to canonical (standard skeleton) bones, with
// some restrictions -
// 1. We only use each target bone once for the unique set, and nodes higher up the tree have priority.
// 2. If a bone has no target, it inherits target from the parent.
// See logic here: https://github.com/0ad/0ad/blob/358825ebbfa071df9590b523951989b5e5f45e3c/source/collada/StdSkeletons.cpp#L101
void GetAllSkeletonBoneMappings(XMLHandle node, const std::string parent_target, const std::map<std::string, int>& canonical_bones,
                                std::map<std::string, int>* mappings, std::map<std::string, int>* unique_mappings,
                                std::set<int>* unused_target_bones) {
  auto children = GetAllChildrenElements(node, "bone");
  for (auto& child : children) {
    std::string name = child.ToElement()->Attribute("name");
    std::string target = parent_target;
    auto targets = GetAllChildrenElements(child, "target");
    if (!targets.empty()) {
      target = GetTextContent(targets[0]);
    }

    auto it = canonical_bones.find(target);
    if (it == canonical_bones.end()) {
      LOG_ERROR("Bone % has target %, which is not in the canonical set", name, target);
      continue;
    }

    int canonical_id = it->second;
    (*mappings)[name] = canonical_id;

    if (unused_target_bones->find(canonical_id) != unused_target_bones->end()) {
      (*unique_mappings)[name] = canonical_id;
      unused_target_bones->erase(canonical_id);
    } else {
      // This target bone has already been used.
      (*unique_mappings)[name] = -1;
    }

    GetAllSkeletonBoneMappings(child, target, canonical_bones, mappings, unique_mappings, unused_target_bones);
  }
}

void MakeSkeleton(const std::string& skeleton_path) {
  static DoneTracker done_tracker;
  if (done_tracker.ShouldSkip(skeleton_path)) {
    return;
  }
  std::string full_path = std::string(kInputPrefix) + kSkeletonPathPrefix + skeleton_path + ".xml";
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing skeleton % at %", skeleton_path, full_path);
  TinyXMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open: ") + full_path);
  }

  XMLHandle root = XMLHandle(xml_doc.FirstChildElement("skeletons"));

  auto standard_skeletons = GetAllChildrenElements(root, "standard_skeleton");
  auto skeletons = GetAllChildrenElements(root, "skeleton");

  if (standard_skeletons.size() != 1) {
    LOG_ERROR("Found % standard_skeleton (expecting 1)", standard_skeletons.size());
    return;
  }

  if (skeletons.empty()) {
    LOG_ERROR("No non-standard skeleton found (expecting >=1)", skeletons.size());
    return;
  }

  std::string canonical_id = standard_skeletons[0].ToElement()->Attribute("id");

  std::vector<std::string> standard_skeleton_bones;
  GetAllStandardSkeletonBones(standard_skeletons[0], &standard_skeleton_bones);

  std::vector<flatbuffers::Offset<data::SkeletonMapping>> skeleton_mapping_offsets;
  for (auto& skeleton : skeletons) {
    std::string skeleton_target = skeleton.ToElement()->Attribute("target");
    if (skeleton_target != canonical_id) {
      LOG_ERROR("Non-standard skeleton has target % (expecting %)", skeleton_target, canonical_id);
      return;
    }

    auto identifiers = GetAllChildrenElements(skeleton, "identifier");
    if (identifiers.size() != 1) {
      LOG_ERROR("Found % identifiers (expecting 1)", identifiers.size());
      return;
    }

    auto roots = GetAllChildrenElements(identifiers[0], "root");
    if (roots.size() != 1) {
      LOG_ERROR("Found % roots (expecting 1)", roots.size());
      return;
    }

    std::string root_name = GetTextContent(roots[0]);

    std::map<std::string, int> canonical_bones;
    std::map<std::string, int> mappings;
    std::map<std::string, int> unique_mappings;
    std::set<int> unused_target_bones;

    for (int i = 0; i < static_cast<int>(standard_skeleton_bones.size()); ++i) {
      canonical_bones[standard_skeleton_bones[i]] = i;
      unused_target_bones.insert(i);
    }

    GetAllSkeletonBoneMappings(skeleton, ""s, canonical_bones, &mappings, &unique_mappings, &unused_target_bones);

    std::vector<std::string> bone_names;
    std::vector<int> mapping_ids;
    std::vector<int> mapping_unique_ids;

    for (const auto& mapping : mappings) {
      LOG_DEBUG("% -> % (%)", mapping.first, mapping.second, standard_skeleton_bones[mapping.second]);
      bone_names.push_back(mapping.first);
      mapping_ids.push_back(mapping.second);
      mapping_unique_ids.push_back(unique_mappings[mapping.first]);
    }

    skeleton_mapping_offsets.push_back(data::CreateSkeletonMapping(
        builder,
        /*root_name=*/builder.CreateString(root_name),
        /*bone_names=*/builder.CreateVectorOfStrings(bone_names),
        /*canonical_ids=*/builder.CreateVector(mapping_ids),
        /*unique_canonical_ids=*/builder.CreateVector(mapping_unique_ids)
    ));
  }

  auto skeleton = data::CreateSkeleton(
      builder,
      /*path=*/builder.CreateString(RemoveExtension(skeleton_path)),
      /*id=*/builder.CreateString(canonical_id),
      /*canonical_bones=*/builder.CreateVectorOfStrings(standard_skeleton_bones),
      /*mappings=*/builder.CreateVector(skeleton_mapping_offsets)
      );
  builder.Finish(skeleton);

  // We have to save a copy of the buffer to back our list of skeletons, because
  // the builder is about to go out of scope.
  std::vector<uint8_t> new_buffer(builder.GetSize());
  auto* buffer_ptr = builder.GetBufferPointer();
  std::copy(buffer_ptr, buffer_ptr + builder.GetSize(), new_buffer.begin());
  g_skeleton_buffers.push_back(std::move(new_buffer));
  g_skeletons.push_back(data::GetSkeleton(g_skeleton_buffers.back().data()));
  
  WriteFB(std::string(kOutputPrefix) + kSkeletonPathPrefix + RemoveExtension(skeleton_path),
          builder);
}

int main(int /*argc*/, char** /*argv*/) {
  logger.LogToStdOutLevel(Logger::eLevel::INFO);
  auto start_time = GetTimeUs();
  FCollada::Initialize();

  unsigned threads_to_use = std::thread::hardware_concurrency();
  if (threads_to_use <= 1) {
    threads_to_use = 1;
  }

  // Do skeletons first in the main thread because they are fast and we need them for
  // parsing meshes. There is no easy way to find all used skeletons in actors without
  // parsing all the skeleton files. Since there aren't that many of them, we just do
  // them all.
  std::vector<std::string> skeletons;

  for (const auto &entry : std::filesystem::directory_iterator(std::string(kInputPrefix)
      + kSkeletonPathPrefix)) {
    if (entry.path().extension() == ".xml") {
      MakeSkeleton(entry.path().stem());
    }
  }

  LOG_INFO("Using % threads per thread pool", threads_to_use);

  g_texture_animation_pool = std::make_unique<ThreadPool>(threads_to_use);
  g_texture_animation_pool->Run();

  g_parser_pool = std::make_unique<ThreadPool>(threads_to_use);
  g_parser_pool->Run();

  for (const auto& path : kTestTerrainPaths) {
    g_parser_pool->Push([path]() { MakeTerrain(path); });
  }

  for (const auto& path : kTestActorPaths) {
    // We only parallelize in the unit of root actors (not props) so
    // we know when all pushes have happened and we can tell the
    // threadpool to exit.
    g_parser_pool->Push([path]() { MakeActor(path); });
  }

  // We have to wait for the parser pool to finish first so we know we won't be adding
  // more textures.
  g_parser_pool.reset();

  g_texture_animation_pool.reset();

  FCollada::Release();
  LOG_INFO("Took % seconds", (GetTimeUs() - start_time) / 1000000.0f);
  return 0;
}
