from conan import ConanFile
from conan.tools.cmake import cmake_layout


class OpenToonDependencies(ConanFile):
    """Pinned infrastructure dependencies. Qt is supplied by the desktop SDK."""

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("sqlite3/3.53.4")
        self.requires("nlohmann_json/3.12.0")
        self.requires("catch2/3.15.0")
        self.requires("json-c/0.18")
        self.requires("zstd/1.5.7")
        self.requires("openssl/3.5.8")
        self.requires("miniaudio/0.11.25")

    def layout(self):
        cmake_layout(self)
