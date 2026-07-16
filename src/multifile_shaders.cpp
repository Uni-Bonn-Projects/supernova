#include "framework/gl/program.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace sn {

typedef struct {
  std::vector<std::string> paths;

  bool is_loaded(const std::string &path) {
    return std::find(paths.begin(), paths.end(), path) != paths.end();
  }

  /** Marks a path as loaded. Asserts that the given path is not yet loaded. */
  void panicing_set_loaded(const std::string &path) {
    assert(!is_loaded(path));
    paths.push_back(path);
  }
} LoadedShaders;

std::optional<std::string> load_shader(const std::string &path,
                                       const std::string &root_path,
                                       LoadedShaders &shaders_loaded) {
  shaders_loaded.panicing_set_loaded(path);

  const auto shader_path = root_path + "/" + path;
  std::ifstream file(shader_path);
  std::string ret;

  if (!file.is_open()) {
    std::cout << "[load_fragment_shader] failed to open " << shader_path
              << std::endl;
    return std::nullopt;
  }

  std::string buf;
  while (std::getline(file, buf)) {
    if (buf.rfind("#include \"") == 0) {
      // we are in a include line
      // add the correct files contents if this is the first include of that
      // file also do some rudimentary error checking

      // check for correct ending syntax
      if (buf.back() != '"' || buf.length() <= 11) {
        std::cout << "[load_fragment_shader] Invalid syntax at #include! "
                     "Expected something like `#include \"my_shader.glsl\"`! "
                     "Error in file: "
                  << shader_path << std::endl;
        return std::nullopt;
      }

      // the actual filename should begin after 10 chars and include the second
      // last char
      auto include_path = buf.substr(10, buf.length() - 10 - 1);

      // ignore #include if file already got included elsewhere
      if (shaders_loaded.is_loaded(include_path)) {
        continue;
      }

      // load of include successful => append contents to ret and go to next
      //                               line
      // else => fail too
      auto include_contents =
          load_shader(include_path, root_path, shaders_loaded);
      if (!include_contents.has_value()) {
        return std::nullopt;
      } else {
        ret.append(include_contents.value() + "\n");
      }
    } else {
      // everything that is not a #include just gets appended as is
      ret.append(buf + "\n");
    }
  }

  return ret;
}

void load_shaders(Program &program, const std::string &root_path,
                  const std::string &vertex_path,
                  const std::string &fragment_path) {
  LoadedShaders loaded_vertex_shaders;
  auto vertex_shader =
      load_shader(vertex_path, root_path, loaded_vertex_shaders);

  LoadedShaders loaded_fragment_shaders;
  auto fragment_shader =
      load_shader(fragment_path, root_path, loaded_fragment_shaders);

  if (vertex_shader.has_value() && fragment_shader.has_value()) {
    program.loadSource(vertex_shader.value(), fragment_shader.value());
  }
}

}; // namespace sn
