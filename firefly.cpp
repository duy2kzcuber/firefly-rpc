#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#else
#  include <cerrno>
#  include <cstdlib>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <unistd.h>
#endif

namespace {
constexpr char kClientId[] = "585817538412150794";
constexpr auto kReconnectDelay = std::chrono::seconds(30);

enum class Opcode : std::uint32_t { Handshake = 0, Frame = 1, Close = 2, Ping = 3, Pong = 4 };

class Connection {
public:
    Connection() = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection() { close(); }

    bool open() {
#ifdef _WIN32
        for (int i = 0; i < 10; ++i) {
            const std::string path = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
            HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                   OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE) { handle_ = h; return true; }
        }
#else
        std::string runtimeDir;
        if (const char* value = std::getenv("XDG_RUNTIME_DIR")) runtimeDir = value;
        if (runtimeDir.empty()) runtimeDir = "/run/user/" + std::to_string(::getuid());
        for (int i = 0; i < 10; ++i) {
            const std::string suffix = "discord-ipc-" + std::to_string(i);
            const std::string paths[] = {runtimeDir + "/" + suffix, "/tmp/" + suffix};
            for (const auto& path : paths) {
                int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
                if (fd < 0) continue;
                sockaddr_un address{};
                address.sun_family = AF_UNIX;
                std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
                if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
                    fd_ = fd; return true;
                }
                ::close(fd);
            }
        }
#endif
        return false;
    }

    bool sendFrame(Opcode opcode, const std::string& payload) {
        const std::uint32_t header[2] = {static_cast<std::uint32_t>(opcode),
                                         static_cast<std::uint32_t>(payload.size())};
        return writeAll(reinterpret_cast<const char*>(header), sizeof(header)) &&
               writeAll(payload.data(), payload.size());
    }

    bool readFrame(std::uint32_t& opcode, std::string& payload) {
        std::uint32_t header[2]{};
        if (!readAll(reinterpret_cast<char*>(header), sizeof(header))) return false;
        opcode = header[0];
        if (header[1] > 1024 * 1024) return false;
        payload.resize(header[1]);
        return payload.empty() || readAll(payload.data(), payload.size());
    }

    void close() {
#ifdef _WIN32
        if (handle_ != INVALID_HANDLE_VALUE) { CloseHandle(handle_); handle_ = INVALID_HANDLE_VALUE; }
#else
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
#endif
    }

private:
    bool writeAll(const char* data, std::size_t size) {
        while (size) {
#ifdef _WIN32
            DWORD written = 0;
            if (!WriteFile(handle_, data, static_cast<DWORD>(size), &written, nullptr) || !written) return false;
            const auto count = static_cast<std::size_t>(written);
#else
            const auto n = ::write(fd_, data, size);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false;
            const auto count = static_cast<std::size_t>(n);
#endif
            data += count; size -= count;
        }
        return true;
    }

    bool readAll(char* data, std::size_t size) {
        while (size) {
#ifdef _WIN32
            DWORD received = 0;
            if (!ReadFile(handle_, data, static_cast<DWORD>(size), &received, nullptr) || !received) return false;
            const auto count = static_cast<std::size_t>(received);
#else
            const auto n = ::read(fd_, data, size);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false;
            const auto count = static_cast<std::size_t>(n);
#endif
            data += count; size -= count;
        }
        return true;
    }

#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif
};

bool runRpc(std::int64_t startedAt) {
    Connection connection;
    if (!connection.open()) return false;

    if (!connection.sendFrame(Opcode::Handshake,
            std::string("{\"v\":1,\"client_id\":\"") + kClientId + "\"}")) return false;

    bool ready = false;
    while (!ready) {
        std::uint32_t opcode = 0;
        std::string payload;
        if (!connection.readFrame(opcode, payload)) return false;
        if (opcode == static_cast<std::uint32_t>(Opcode::Close)) return false;
        if (opcode == static_cast<std::uint32_t>(Opcode::Ping)) {
            if (!connection.sendFrame(Opcode::Pong, payload)) return false;
        }
        if (payload.find("\"evt\":\"READY\"") != std::string::npos) ready = true;
    }

    std::cout << "RPC connected" << std::endl;
    const std::string activity =
      std::string("{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":") +
      std::to_string(
#ifdef _WIN32
        GetCurrentProcessId()
#else
        ::getpid()
#endif
      ) + ",\"activity\":{\"timestamps\":{\"start\":" + std::to_string(startedAt) +
      "},\"buttons\":["
          "{\"label\":\"GitHub\",\"url\":\"https://github.com/duy2kzcuber\"},"
          "{\"label\":\"Facebook\",\"url\":\"https://www.facebook.com/15792617.d/\"}"
      "] }},\"nonce\":\"firefly-activity\"}";
    if (!connection.sendFrame(Opcode::Frame, activity)) return false;

    // Keep reading so Discord close/error events and pipe disconnects are noticed.
    while (true) {
        std::uint32_t opcode = 0;
        std::string payload;
        if (!connection.readFrame(opcode, payload)) return false;
        if (opcode == static_cast<std::uint32_t>(Opcode::Close)) return false;
        if (opcode == static_cast<std::uint32_t>(Opcode::Ping) &&
            !connection.sendFrame(Opcode::Pong, payload)) return false;
    }
}
} // namespace

int main() {
    const auto startedAt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    while (true) {
        if (!runRpc(startedAt)) std::cout << "RPC disconnected or unavailable" << std::endl;
        std::this_thread::sleep_for(kReconnectDelay);
        std::cout << "Reconnecting RPC..." << std::endl;
    }
}
