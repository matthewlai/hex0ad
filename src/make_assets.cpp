#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "logger.h"
#include "utils.h"

#include "actor_generated.h"
#include "mesh_generated.h"

#include "flatbuffers/flatbuffers.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "gli/gli.hpp"
#pragma GCC diagnostic pop

#include "lodepng/lodepng.h"
#include "tinyxml2/tinyxml2.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLHandle;
using tinyxml2::XMLNode;

namespace {
//static constexpr const char* kTestActorPath = "structures/persians/fortress.xml";
static constexpr const char* kTestActorPaths[] = {
    "structures/persians/stable.xml",
    //"units/athenians/hero_infantry_javelinist_iphicrates.xml"
    };

constexpr int kFlatBuilderInitSize = 4 * 1024 * 1024;

static constexpr const char* kInputPrefix = "0ad_assets/";
static constexpr const char* kOutputPrefix = "assets/";
static constexpr const char* kActorPathPrefix = "art/actors/";
static constexpr const char* kMeshPathPrefix = "art/meshes/";
static constexpr const char* kTexturePathPrefix = "art/textures/skins/";
static constexpr const char* kVariantPathPrefix = "art/variants/";

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

void WriteToFile(const std::string& path, uint8_t* buf, std::size_t size) {
  std::size_t last_separator = path.find_last_of('/');
  std::string dir = path.substr(0, last_separator);
  std::string filename = path.substr(last_separator + 1);
  MkdirIfNotExist(dir);
  std::ofstream of(
      path.c_str(),
      std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  of.write(reinterpret_cast<const char*>(buf), size);
}

XMLNode* DeepCopy(XMLNode* node, XMLDocument* doc) {
    XMLNode* ret = node->ShallowClone(doc);
    for (XMLElement* child = node->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
      ret->InsertEndChild(DeepCopy(child, doc));
    }

    return ret;
}

// Variants can have a "file" attribute that "includes" another file.
// Here we copy the file into the main tree (if applicable).
void ProcessVariantIncludes(XMLElement* element, XMLDocument* doc) {
  if (!element) {
    return;
  }
  
  const char* file_attrib = element->Attribute("file");
  if (!file_attrib) {
    return;
  }

  std::string full_path = std::string(kInputPrefix) + kVariantPathPrefix + file_attrib;
  XMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open for variant inclusion: ") + full_path);
  }

  XMLElement* root = xml_doc.FirstChildElement("variant");
  if (!root) {
    throw std::runtime_error(std::string("Variant include file ") + full_path + " does not have <variant> root");
  }
  ProcessVariantIncludes(root, doc);

  XMLElement* child = root->FirstChildElement();
  while (child) {
    element->InsertEndChild(DeepCopy(child, doc));
    child = child->NextSiblingElement();
  }
}
}

// Returns name and transform of attachment points.
void GetAttachmentPoints(aiNode* node, std::vector<std::pair<std::string, aiMatrix4x4>>* points, aiMatrix4x4 current_matrix) {
  if (!node) {
    return;
  }

  // If we have a node with mesh, this is the root and we can reset the current_matrix.
  // We have already checked by this point that there is only one mesh in the scene.
  if (node->mNumMeshes > 0) {
    current_matrix = aiMatrix4x4();
    points->push_back(std::make_pair(std::string("root"), current_matrix));
  } else {
    current_matrix = current_matrix * node->mTransformation;
    points->push_back(std::make_pair(std::string(node->mName.C_Str()), current_matrix));
  }

  for (unsigned int i = 0; i < node->mNumChildren; ++i) {
    GetAttachmentPoints(node->mChildren[i], points, current_matrix);
  }
}

// textures are stored next to meshes in actor XML, but we put them in with
// meshes.
void ParseMesh(const std::string& mesh_path) {
  std::string full_path = std::string(kInputPrefix) + kMeshPathPrefix + mesh_path;
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing mesh % at %", mesh_path, full_path);
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(full_path,
    aiProcess_CalcTangentSpace       |
    aiProcess_Triangulate            |
    aiProcess_JoinIdenticalVertices  |
    aiProcess_GenNormals             |
    aiProcess_FlipUVs                |
    aiProcess_SortByPType);
  if (!scene) {
    LOG_ERROR("Failed to parse %: %", mesh_path, importer.GetErrorString());
    return;
  }

  if (scene->mNumMeshes != 1) {
    LOG_ERROR("We have % meshes (expecting 1)", scene->mNumMeshes);
    return;
  }

  auto* mesh = scene->mMeshes[0];
  if (!mesh->HasPositions()) {
    LOG_ERROR("We have no positions?");
    return;
  }

  if (!mesh->HasNormals()) {
    LOG_ERROR("We have no normals?");
    return;
  }

  for (int ch = 0; ch < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++ch) {
    if (mesh->mNumUVComponents[ch] > 0) {
      LOG_DEBUG("Texture channel %: %", ch, mesh->mNumUVComponents[ch]);
    } else {
      break;
    }
  }

  // Only deal with 1 texture channel for now (is the second one for ambient occlusion map?).
  std::vector<float> tex_coords(mesh->mNumVertices * 2, 0.0f);
  constexpr int kTexCoordsChannel = 0;
  if (mesh->mNumUVComponents[kTexCoordsChannel] == 2) {
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
      auto& uv = mesh->mTextureCoords[kTexCoordsChannel][i];
      tex_coords[i * 2] = uv[0];
      tex_coords[i * 2 + 1] = uv[1];
    }
  }

  std::vector<float> ao_tex_coords(mesh->mNumVertices * 2, 0.0f);
  constexpr int kAoTexCoordsChannel = 1;
  if (mesh->mNumUVComponents[kAoTexCoordsChannel] == 2) {
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
      auto& uv = mesh->mTextureCoords[kAoTexCoordsChannel][i];
      ao_tex_coords[i * 2] = uv[0];
      ao_tex_coords[i * 2 + 1] = uv[1];
    }
  }

  LOG_INFO("% faces", mesh->mNumFaces);
  std::vector<uint32_t> vertex_indices(mesh->mNumFaces * 3);
  for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
    auto* face = &mesh->mFaces[i];
    if (face->mNumIndices != 3) {
      LOG_ERROR("Non-triangle with % indices detected", face->mNumIndices);
      return;
    }
    vertex_indices[i * 3] = face->mIndices[0];
    vertex_indices[i * 3 + 1] = face->mIndices[1];
    vertex_indices[i * 3 + 2] = face->mIndices[2];
  }
  LOG_INFO("% vertices", mesh->mNumVertices);
  std::vector<float> vertices(mesh->mNumVertices * 3);
  for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
    auto& vec = mesh->mVertices[i];
    if (i < 100) {
      LOG_INFO("% % %", vec[0], vec[1], vec[2]);
    }
    vertices[i * 3] = vec[0];
    vertices[i * 3 + 1] = vec[1];
    vertices[i * 3 + 2] = vec[2];
  }

  std::vector<float> normals(mesh->mNumVertices * 3);
  for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
    auto& vec = mesh->mNormals[i];
    normals[i * 3] = vec[0];
    normals[i * 3 + 1] = vec[1];
    normals[i * 3 + 2] = vec[2];
  }

  std::vector<float> tangents(mesh->mNumVertices * 3, 0.0f);
  if (mesh->HasTangentsAndBitangents()) {
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
      auto& vec = mesh->mTangents[i];
      tangents[i * 3] = vec[0];
      tangents[i * 3 + 1] = vec[1];
      tangents[i * 3 + 2] = vec[2];
    }
  }

  std::vector<std::pair<std::string, aiMatrix4x4>> points;
  GetAttachmentPoints(scene->mRootNode, &points, aiMatrix4x4());

  std::vector<std::string> attachment_point_names;
  std::vector<float> attachment_point_transforms;

  for (const auto& pair : points) {
    attachment_point_names.push_back(pair.first);
    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        attachment_point_transforms.push_back(pair.second[row][col]);
      }
    }
  }

  auto mesh_fb = data::CreateMesh(
    builder,
    /*path=*/builder.CreateString(RemoveExtension(mesh_path)),
    /*vertex_indices=*/builder.CreateVector(vertex_indices),
    /*vertices=*/builder.CreateVector(vertices),
    /*normals=*/builder.CreateVector(normals),
    /*tangents=*/builder.CreateVector(tangents),
    /*tex_coords=*/builder.CreateVector(tex_coords),
    /*ao_tex_coords=*/builder.CreateVector(ao_tex_coords),
    /*attachment_point_names=*/builder.CreateVectorOfStrings(attachment_point_names),
    /*attachment_point_transforms=*/builder.CreateVector(attachment_point_transforms)
    );
  builder.Finish(mesh_fb);
  WriteToFile(std::string(kOutputPrefix) + kMeshPathPrefix + RemoveExtension(mesh_path) + ".fb",
              builder.GetBufferPointer(), builder.GetSize());
}

void SaveTexture(const std::string& texture_path) {
  std::string full_path = std::string(kInputPrefix) + kTexturePathPrefix + texture_path;
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Saving texture % at %", texture_path, full_path);
  std::ifstream is(full_path);
  auto file_content = ReadWholeFile(full_path);
  std::string old_extension = Extension(texture_path);
  std::string output_path =
      std::string(kOutputPrefix) + kTexturePathPrefix + RemoveExtension(texture_path) + ".png";
  if (old_extension == "png") {
    WriteToFile(output_path, file_content.data(), file_content.size());
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
    std::vector<uint8_t> uncompressed;
    for (int y = 0; y < texture.extent().y; ++y) {
      for (int x = 0; x < texture.extent().x; ++x) {
        glm::vec4 data = read_sampler.texel_fetch(gli::texture2d::extent_type(x, y), 0);
        for (int p = 0; p < 4; ++p) {
          uncompressed.push_back(data[p] * 255);
        }
      }
    }
    std::vector<uint8_t> png_data;
    uint32_t error = lodepng::encode(
        png_data, reinterpret_cast<const uint8_t*>(uncompressed.data()), texture.extent().x, texture.extent().y);
    if (error) {
      LOG_ERROR("Failed to encode PNG: %", lodepng_error_text(error));
      return;
    }
    WriteToFile(output_path, png_data.data(), png_data.size());
  } else {
    LOG_ERROR("Unknown texture extension: %", old_extension);
    return;
  }
}

void MakeActor(const std::string& actor_path) {
  std::string full_path = std::string(kInputPrefix) + kActorPathPrefix + actor_path;
  flatbuffers::FlatBufferBuilder builder(kFlatBuilderInitSize);
  LOG_INFO("Parsing actor % at %", actor_path, full_path);
  XMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open: ") + full_path);
  }

  XMLHandle root = XMLHandle(xml_doc.FirstChildElement("actor"));
  std::vector<XMLHandle> xml_groups = GetAllChildrenElements(root, "group");

  if (xml_groups.empty()) {
    throw std::runtime_error(full_path + " has no groups?");
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
        mesh_path = meshes[0].FirstChild().ToNode()->Value();
        ParseMesh(mesh_path);
        mesh_path = RemoveExtension(mesh_path) + ".fb";
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
          SaveTexture(texture_file);
          texture_offsets.push_back(data::CreateTexture(
              builder,
              /*name=*/builder.CreateString(texture_name),
              /*file=*/builder.CreateString(RemoveExtension(texture_file))
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
                /*actor=*/builder.CreateString(RemoveExtension(actor) + ".fb"),
                /*attachpoint=*/builder.CreateString(attachpoint)));
            LOG_DEBUG("Prop: % (%)", actor, attachpoint);
            MakeActor(actor);
          }
        }
      }

      std::string name;
      if (xml_variant.ToElement()->Attribute("name")) {
        name = xml_variant.ToElement()->Attribute("name");
      }

      variants.push_back(data::CreateVariant(
          builder,
          /*name=*/builder.CreateString(name),
          /*frequency=*/frequency,
          /*mesh_path=*/builder.CreateString(mesh_path),
          /*props=*/builder.CreateVector(props_offsets),
          /*textures=*/builder.CreateVector(texture_offsets)));
    }
    groups.push_back(CreateGroup(builder, /*variants=*/builder.CreateVector(variants)));
  }
  auto actor = data::CreateActor(
      builder,
      /*path=*/builder.CreateString(RemoveExtension(actor_path)),
      /*groups=*/builder.CreateVector(groups));
  builder.Finish(actor);
  WriteToFile(std::string(kOutputPrefix) + kActorPathPrefix + RemoveExtension(actor_path) + ".fb",
              builder.GetBufferPointer(), builder.GetSize());
}

int main(int /*argc*/, char** /*argv*/) {
  logger.LogToStdOutLevel(Logger::eLevel::INFO);
  for (const auto& path : kTestActorPaths) {
    MakeActor(path);
  }
  return 0;
}
