#include "logger.h"

#include "tinyxml2/tinyxml2.h"

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLHandle;

namespace {
const char* kTestActorPath = "structures/britons/fortress.xml";

static constexpr const char* kActorPathPrefix = "0ad_assets/art/actors/";
static constexpr const char* kMeshPathPrefix = "0ad_assets/art/meshes/";
}

int main(int /*argc*/, char** /*argv*/) {
  logger.LogToStdOutLevel(Logger::eLevel::INFO);
  
  return 0;
}
