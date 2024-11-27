from conan import ConanFile

class VideoserverRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("fmt/11.0.2", options={"shared": False}, force=True)
        self.requires("spdlog/1.15.0", options={"shared": False})
        self.requires("nlohmann_json/3.11.3")
        self.requires("boost/1.86.0",
                      options={"shared": False, "without_python": True})
        self.requires("flatbuffers/24.3.25", options={"shared": False})
        self.requires("protobuf/5.27.0", options={"shared": False})
        self.requires("cpptrace/0.7.3", options={"shared": False})
        # Testing
        self.requires("catch2/3.7.1", options={"shared": False})
