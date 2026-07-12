#include "app/application.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static std::filesystem::path desktop_entry_path() {
    const char* home = std::getenv("HOME");
    if (!home) {
        return {};
    }
    return std::filesystem::path(home) / ".local/share/applications/sord.desktop";
}

static bool desktop_entry_exists() {
    auto path = desktop_entry_path();
    return !path.empty() && std::filesystem::exists(path);
}

static bool write_desktop_entry(const std::filesystem::path& executable, std::string& error) {
    auto path = desktop_entry_path();
    if (path.empty()) {
        error = "HOME environment variable is not set.";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        error = "Unable to create desktop entry directory.";
        return false;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        error = "Unable to write desktop entry file.";
        return false;
    }

    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=Sord\n";
    out << "Exec=\"" << executable.string() << "\" %F\n";
    out << "Terminal=true\n";
    out << "Categories=Utility;TextEditor;\n";
    out << "MimeType=text/plain;text/markdown;text/x-csrc;text/x-c++src;text/x-java;text/x-python;text/x-shellscript;application/pdf;application/javascript;application/json;application/xml;application/x-sord;application/octet-stream;\n";
    out.close();

    if (!out) {
        error = "Unable to write desktop entry file.";
        return false;
    }

    std::system(("update-desktop-database " + path.parent_path().string()).c_str());
    std::system("xdg-mime default sord.desktop text/plain");
    std::system("xdg-mime default sord.desktop text/markdown");
    std::system("xdg-mime default sord.desktop application/pdf");
    std::system("xdg-mime default sord.desktop application/x-sord");
    std::system("xdg-mime default sord.desktop application/json");
    std::system("xdg-mime default sord.desktop application/xml");
    return true;
}

static std::filesystem::path ipc_socket_path() {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0] != '\0') {
        return std::filesystem::path(runtime_dir) / ("sord-ipc-" + std::to_string(geteuid()) + ".sock");
    }
    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') {
        return std::filesystem::path(home) / ".cache" / ("sord-ipc-" + std::to_string(geteuid()) + ".sock");
    }
    return std::filesystem::path("/tmp") / ("sord-ipc-" + std::to_string(geteuid()) + ".sock");
}

static bool send_open_request(const std::filesystem::path& socket_path, const std::string& file_path) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::string path = socket_path.string();
    if (path.size() >= sizeof(addr.sun_path)) {
        close(sock);
        return false;
    }

    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(sock);
        return false;
    }

    std::string payload = file_path;
    payload.push_back('\n');
    ssize_t sent = write(sock, payload.data(), payload.size());
    close(sock);
    return sent == static_cast<ssize_t>(payload.size());
}

static void remove_stale_socket(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        std::filesystem::remove(path, ec);
    }
}

int main(int argc, char* argv[]) {
    const auto exec_path = std::filesystem::absolute(argv[0]);
    const auto socket_path = ipc_socket_path();

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--register") {
            std::string error;
            if (!write_desktop_entry(exec_path, error)) {
                std::fprintf(stderr, "Registration failed: %s\n", error.c_str());
                return 1;
            }
            return 0;
        }

        if (!desktop_entry_exists()) {
            std::string error;
            write_desktop_entry(exec_path, error);
        }

        if (send_open_request(socket_path, arg)) {
            return 0;
        }

        remove_stale_socket(socket_path);
        return sord::app::Application(std::filesystem::path(arg)).run();
    }

    if (!desktop_entry_exists()) {
        std::string error;
        write_desktop_entry(exec_path, error);
    }
    sord::app::Application app(socket_path, true);
    return app.run();
}
