#include "actor.h"

#include <stdexcept>
#include <vector>

#include "tinyxml2/tinyxml2.h"

#include "logger.h"

namespace {

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLHandle;

static constexpr const char* kActorPathPrefix = "assets/art/actors/";
static constexpr const char* kMeshPathPrefix = "assets/art/meshes/";

std::vector<XMLHandle> GetAllChildrenElements(XMLHandle root, const std::string& elem) {
  std::vector<XMLHandle> ret;
  XMLHandle it = root.FirstChildElement(elem.c_str());
  while (it.ToNode() != nullptr) {
    ret.push_back(it);
    it = it.NextSiblingElement(elem.c_str());
  }
  return ret;
}
}

Actor::Actor(ActorTemplate* actor_template) : template_(actor_template) {
  for (int group = 0; group < template_->NumGroups(); ++group) {
    std::uniform_int_distribution<> dist(0, template_->NumVariants(group) - 1);
    variant_selections_.push_back(dist(actor_template->Rng()));
  }
}

ActorTemplate::ActorTemplate(const std::string& actor_path) {
  std::string full_path = std::string(kActorPathPrefix) + actor_path;
  LOG_INFO("Parsing %", full_path);
  XMLDocument xml_doc;
  if (xml_doc.LoadFile(full_path.c_str()) != 0) {
    throw std::runtime_error(std::string("Failed to open: ") + full_path);
  }

  XMLHandle root = XMLHandle(xml_doc.FirstChildElement("actor"));
  std::vector<XMLHandle> xml_groups = GetAllChildrenElements(root, "group");

  if (xml_groups.empty()) {
    throw std::runtime_error(full_path + " has no groups?");
  }

  LOG_INFO("% groups found", xml_groups.size());

  for (auto xml_group : xml_groups) {
    Group group;

    // Each group should have 1 or more variant.
    auto xml_variants = GetAllChildrenElements(xml_group, "variant");
    if (xml_variants.empty()) {
      throw std::runtime_error(full_path + " has a group with no variant");
    }
    LOG_INFO("% variants found", xml_variants.size());

    for (auto xml_variant : xml_variants) {
      Variant variant;

      variant.frequency = xml_variant.ToElement()->FloatAttribute("frequency", 0.0f);

      auto meshes = GetAllChildrenElements(xml_variant, "mesh");
      if (!meshes.empty()) {
        if (meshes.size() != 1) {
          throw std::runtime_error("More than one mesh in a variant?");
        }
        variant.mesh_path = meshes[0].FirstChild().ToNode()->Value();
      }
      group.push_back(std::move(variant));
    }
    groups_.push_back(std::move(group));
  }
}