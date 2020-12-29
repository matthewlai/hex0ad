#ifndef HEX_H
#define HEX_H

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"

#include <cmath>
#include <cstdint>
#include <map>
#include <ostream>
#include <sstream>
#include <utility>

// This class implements hexagon grid operations described here:
// https://www.redblobgames.com/grids/hexagons/
// We use the axial representation, computing the missing dimension in cube where necessary.
class Hex {
 public:
  Hex() : q_(0), r_(0) {}
  Hex(int32_t q, int32_t r) : q_(q), r_(r) {}

  // A Hex has 6 vertices - 0 to 5. We are using pointy-topped hexagons.
  // Calls yield(const glm::vec2& vertex, index) for each vertex.
  template <typename YieldFn>
  void ForEachVertex(float grid_size, float vertex_distance, YieldFn yield) const {
  	const static glm::vec2 offsets[6] = {
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 0 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 0 - 30.0f))),
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 1 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 1 - 30.0f))),
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 2 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 2 - 30.0f))),
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 3 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 3 - 30.0f))),
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 4 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 4 - 30.0f))),
  	  glm::vec2(cos(M_PI / 180.0f * (60.0f * 5 - 30.0f)), sin(M_PI / 180.0f * (60.0f * 5 - 30.0f))),
  	};

  	for (int i = 0; i < 6; ++i) {
  	  yield(Centre(grid_size) + offsets[i] * vertex_distance, i);
  	}
  }
  
  // Generates a "ring" of Hexes at `distance` from this centre hex.
  // Calls yield(const Hex& hex) for each hex.
  template <typename YieldFn>
  void ForEachHexAtDist(int distance, YieldFn yield) const {
  	if (distance == 0) {
  		yield(*this);
  		return;
  	}

  	// We are basically looking at all valid combinations of offsets in cube coordinates with at least
  	// one dim at distance.
  	for (int32_t offset_q : {distance, -distance}) {
  	  ForEachCombination(-offset_q, [&](int32_t offset_r, int32_t offset_s) {
  	  	(void) offset_s;
  	  	yield(Hex(q() + offset_q, r() + offset_r));
  	  });
  	}

  	for (int32_t offset_r : {distance, -distance}) {
  	  ForEachCombination(-offset_r, [&](int32_t offset_s, int32_t offset_q) {
  	 	(void) offset_s;
  	  	yield(Hex(q() + offset_q, r() + offset_r));
  	  });
  	}

  	for (int32_t offset_s : {distance, -distance}) {
  	  ForEachCombination(-offset_s, [&](int32_t offset_q, int32_t offset_r) {
  	  	yield(Hex(q() + offset_q, r() + offset_r));
  	  });
  	}
  }

  glm::vec2 Centre(float grid_size) const {
  	float x = grid_size * (sqrt(3.0f) * q_ + sqrt(3.0f) / 2.0f * r_);
  	float y = grid_size * (3.0f / 2.0f * r_);
  	return glm::vec2(x, y);
  }

  static Hex HexRound(float q_frac, float r_frac) {
    glm::vec3 cube_frac(q_frac, r_frac, -(q_frac + r_frac));
    glm::vec3 cube_rounded = glm::round(cube_frac);
    glm::vec3 diff = glm::abs(cube_frac - cube_rounded);
    if (diff.x > diff.y && diff.x > diff.z) {
      cube_rounded.x = -(cube_rounded.y + cube_rounded.z);
    } else if (diff.y > diff.x && diff.y > diff.z) {
      cube_rounded.y = -(cube_rounded.x + cube_rounded.z);
    }
    return Hex(round(cube_rounded.x), round(cube_rounded.y));
  }

  static Hex CartesianToHex(const glm::vec2& coords, float grid_size) {
    float q_frac = (sqrt(3.0f) / 3.0f * coords.x - 1.0f / 3.0f * coords.y) / grid_size;
    float r_frac = 2.0f / 3.0f * coords.y / grid_size;
    return HexRound(q_frac, r_frac);
  }

  static glm::vec2 SnapToGrid(const glm::vec2& coords, float grid_size) {
    return CartesianToHex(coords, grid_size).Centre(grid_size);
  }

  int32_t q() const { return q_; }
  int32_t r() const { return r_; }
  int32_t s() const { return -(q_ + r_); }

  std::string ToString() const {
  	std::stringstream ss;
  	ss << "Hex(" << q() << "," << r() << "," << s() << ") at (" << Centre(1.0f).x << "," << Centre(1.0f).y << ")";
  	return ss.str();
  }

  bool operator==(const Hex& other) const {
  	return q_ == other.q_ && r_ == other.r_;
  }

  bool operator!=(const Hex& other) const { return !(*this == other); }

 private:
  int32_t q_;
  int32_t r_;

  // Yields all two number pairs up to `sum` that add up to `sum`, skipping (sum, 0) so we don't
  // get a duplicate hex.
  template<typename YieldFn>
  static void ForEachCombination(int32_t sum, YieldFn yield) {
  	int32_t inc = sum >= 0 ? 1 : -1;
  	for (int32_t a = 0; a != sum; a += inc) {
  	  yield(a, sum - a);
  	}
  }
};

inline std::ostream& operator<<(std::ostream& os, const Hex& hex) {
	os << hex.ToString();
	return os;
}

#endif // HEX_H
