#include <windows.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

class PyrightLangserverShim {
public:
    int run(int argc, wchar_t* argv[]) {
        std::optional<std::filesystem::path> commandPath = findCommandPath();
        if (!commandPath.has_value()) {
            std::wcerr << L"pyright-langserver.cmd not found." << std::endl;
            return 1;
        }

        return launchCommand(commandPath.value(), argc, argv);
    }

private:
    std::optional<std::filesystem::path> findCommandPath() const {
        std::vector<std::filesystem::path> candidates = collectCandidates();
        for (const std::filesystem::path& candidate : candidates) {
            if (isValidCommand(candidate)) {
                return std::filesystem::absolute(candidate);
            }
        }

        return std::nullopt;
    }

    std::vector<std::filesystem::path> collectCandidates() const {
        std::vector<std::filesystem::path> candidates;
        std::optional<std::filesystem::path> executableDir = getExecutableDirectory();
        if (executableDir.has_value()) {
            // Check the shim directory first so a colocated deployment wins immediately.
            candidates.push_back(executableDir.value() / L"pyright-langserver.cmd");
        }

        // Fall back to PATH scanning to avoid relying on the current working directory.
        std::wstring rawPath = getEnvironmentValue(L"PATH");
        std::wstringstream stream(rawPath);
        std::wstring segment;
        while (std::getline(stream, segment, L';')) {
            if (segment.empty()) {
                continue;
            }

            candidates.emplace_back(std::filesystem::path(segment) / L"pyright-langserver.cmd");
        }

        return candidates;
    }

    bool isValidCommand(const std::filesystem::path& candidate) const {
        std::error_code errorCode;
        if (!std::filesystem::exists(candidate, errorCode) || errorCode) {
            return false;
        }

        return std::filesystem::is_regular_file(candidate, errorCode) && !errorCode;
    }

    int launchCommand(const std::filesystem::path& commandPath, int argc, wchar_t* argv[]) const {
        std::filesystem::path cmdExePath = findCmdExePath();
        if (cmdExePath.empty()) {
            std::wcerr << L"cmd.exe not found." << std::endl;
            return 1;
        }

        // .cmd files are shell scripts, so they must be launched through cmd.exe.
        std::wstring commandLine = buildCmdCommandLine(cmdExePath, commandPath, argc, argv);
        std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back(L'\0');

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION processInfo{};
        BOOL created = CreateProcessW(
            cmdExePath.c_str(),
            mutableCommandLine.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

        if (!created) {
            std::wcerr << L"Failed to start "
                       << commandPath.c_str()
                       << L", error="
                       << GetLastError()
                       << std::endl;
            return 1;
        }

        CloseHandle(processInfo.hThread);
        WaitForSingleObject(processInfo.hProcess, INFINITE);

        DWORD exitCode = 1;
        if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
            std::wcerr << L"Failed to read child exit code, error="
                       << GetLastError()
                       << std::endl;
            CloseHandle(processInfo.hProcess);
            return 1;
        }

        CloseHandle(processInfo.hProcess);
        return static_cast<int>(exitCode);
    }

    std::filesystem::path findCmdExePath() const {
        wchar_t buffer[MAX_PATH];
        DWORD result = SearchPathW(
            nullptr,
            L"cmd.exe",
            nullptr,
            MAX_PATH,
            buffer,
            nullptr
        );

        if (result == 0 || result >= MAX_PATH) {
            return {};
        }

        return std::filesystem::path(buffer);
    }

    std::wstring buildCmdCommandLine(
        const std::filesystem::path&,
        const std::filesystem::path& commandPath,
        int argc,
        wchar_t* argv[]
    ) const {
        // Use an absolute path to the real shim so Windows path resolution cannot loop back to us.
        std::wstring innerCommand = quoteForCmd(commandPath.wstring());
        for (int index = 1; index < argc; ++index) {
            innerCommand.push_back(L' ');
            innerCommand.append(quoteForCmdArgument(argv[index]));
        }

        std::wstring commandLine;
        commandLine.reserve(256);
        commandLine.append(L"/d /s /c \"");
        commandLine.append(innerCommand);
        commandLine.push_back(L'"');
        return commandLine;
    }

    std::wstring quoteForCmd(const std::wstring& value) const {
        std::wstring quoted = L"\"";
        for (wchar_t ch : value) {
            if (ch == L'"') {
                quoted.append(L"\"\"");
            } else {
                quoted.push_back(ch);
            }
        }
        quoted.push_back(L'"');
        return quoted;
    }

    std::wstring quoteForCmdArgument(const std::wstring& value) const {
        if (value.empty()) {
            return L"\"\"";
        }

        // Escape cmd metacharacters so argv is forwarded without being reinterpreted by the shell.
        std::wstring escaped;
        escaped.reserve(value.size() + 8);
        escaped.push_back(L'"');
        for (wchar_t ch : value) {
            switch (ch) {
            case L'^':
            case L'&':
            case L'|':
            case L'<':
            case L'>':
            case L'(':
            case L')':
            case L'%':
            case L'!':
                escaped.push_back(L'^');
                escaped.push_back(ch);
                break;
            case L'"':
                escaped.append(L"\\\"");
                break;
            default:
                escaped.push_back(ch);
                break;
            }
        }
        escaped.push_back(L'"');
        return escaped;
    }

    std::optional<std::filesystem::path> getExecutableDirectory() const {
        std::wstring executablePath = getExecutablePath();
        if (executablePath.empty()) {
            return std::nullopt;
        }

        return std::filesystem::path(executablePath).parent_path();
    }

    std::wstring getExecutablePath() const {
        std::vector<wchar_t> buffer(MAX_PATH);
        while (true) {
            DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0) {
                return L"";
            }

            if (length < buffer.size() - 1) {
                return std::wstring(buffer.data(), length);
            }

            buffer.resize(buffer.size() * 2);
        }
    }

    std::wstring getEnvironmentValue(const wchar_t* name) const {
        DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
        if (length == 0) {
            return L"";
        }

        std::wstring buffer(length, L'\0');
        DWORD written = GetEnvironmentVariableW(name, buffer.data(), length);
        if (written == 0 || written >= length) {
            return L"";
        }

        buffer.resize(written);
        return buffer;
    }
};

int wmain(int argc, wchar_t* argv[]) {
    PyrightLangserverShim shim;
    return shim.run(argc, argv);
}
