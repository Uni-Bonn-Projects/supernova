#pragma once

#include "framework/gl/program.hpp"
#include <string>

namespace sn {

/** Load a given vertex and fragment shader from path. Their path is seen as
 * relative to the root_path.
 * Multifile files with #include are supported.
 * */
void load_shaders(Program &program, const std::string &root_path,
                  const std::string &vertex_path,
                  const std::string &fragment_path);

}; // namespace sn
