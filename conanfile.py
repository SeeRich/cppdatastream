from conan import ConanFile

# INCREMENT THIS COMMENT # TO REBUILD THE DEPENDENCIES IN GITHUB ACTIONS: 0


class VideoserverRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("fmt/11.0.2", options={"shared": False}, force=True)
        self.requires("spdlog/1.15.0", options={"shared": False})
        self.requires("boost/1.86.0", options={"shared": False, "without_python": True})
        self.requires("cpptrace/0.7.3", options={"shared": False})
        # Testing
        self.requires("catch2/3.7.1", options={"shared": False})
        # Examples
        self.requires("libpcap/1.10.4", options={"shared": False})
